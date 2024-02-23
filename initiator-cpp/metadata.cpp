#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "metadata.h"

class MntpntMap mntpnt_map;
class FilepathMap filepath_map;

#define DMFP_METADATA_INFO
#ifdef DMFP_METADATA_INFO
#define dmfp_metadata_info(str, ...) \
    printf("[INFO] %s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_metadata_info(str, ...) do {} while (0)
#endif

//#define DMFP_METADATA_DEBUG
#ifdef DMFP_METADATA_DEBUG
#define dmfp_metadata_debug(str, ...) \
    printf("[DEBUG] %s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_metadata_debug(str, ...) do {} while (0)
#endif

/* helper functions */
int getSubsystemId(const std::string& subsysnqn) {
    std::regex pattern("subsystem(\\d+)");
    std::smatch match;

    if (std::regex_search(subsysnqn, match, pattern)) {
        return std::stoi(match[1]);
    }

    return -1; // or any other value indicating failure
}

std::string findMntPath(const std::string& device) {
	std::ifstream mounts_file("/proc/self/mounts");
	if (mounts_file.is_open()) {
		std::string line;
		while (std::getline(mounts_file, line)) {
			if (line.find(device) == 0) {
				std::istringstream iss(line);
				std::string dev_path, mnt_path;
				iss >> dev_path >> mnt_path;
				mounts_file.close();
				return mnt_path;
			}
		}
		mounts_file.close();
	}
	return "";
}

std::pair<std::string, std::string> getMountInfo() {
    std::ifstream mounts_file("/proc/self/mounts");
    if (mounts_file.is_open()) {
        std::string line;
        while (std::getline(mounts_file, line)) {
            if (line.find("/dev/nvme") == 0) {
                std::istringstream iss(line);
                std::string dev_path, mnt_path;
                iss >> dev_path >> mnt_path;
                mounts_file.close();
                return {dev_path, mnt_path};
            }
        }
        mounts_file.close();
    }
    return {"", ""};
}

uint64_t getDevId(const std::string& dev_path) {
    try {
        struct stat result;
        if (stat(dev_path.c_str(), &result) == 0) {
            uint64_t combined = result.st_rdev;
            uint64_t major = (combined >> 8) & 0xfff; // Extract upper 8 bits
            uint64_t minor = combined & 0xff;        // Extract lower 8 bits
            return (major << 20) | minor;
        } else {
            std::cerr << "Error: " << dev_path << " does not exist" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to get device numbers for " << dev_path << 
            ": " << e.what() << std::endl;
    }
    return -1;
}

std::pair<uint32_t, uint32_t> getNvmeInfo(const std::string& dev_name) {
	if (dev_name.find("nvme") == std::string::npos) {
		return {-1, -1};
	}

	auto ctrl_id_start = dev_name.find("nvme") + 4;
	auto ns_id_start = dev_name.find("n", ctrl_id_start) + 1;

	auto ctrl_id_end = ns_id_start - 1;
	auto ns_id_end = dev_name.size();

	int ctrl_id = std::stoi(dev_name.substr(ctrl_id_start, ctrl_id_end - ctrl_id_start));
	int ns_id = std::stoi(dev_name.substr(ns_id_start, ns_id_end - ns_id_start));

	std::string nvmeSysfsPath = "/sys/class/nvme/nvme" + std::to_string(ctrl_id);
	std::string transportPath = nvmeSysfsPath + "/transport";

	std::ifstream transportFile(transportPath);
	if (transportFile.is_open()) {
		std::string transport;
		std::getline(transportFile, transport);
		transportFile.close();

		if (transport == "pcie") {
			return {-1, -1};
		}
	} else {
		std::cerr << "Failed to open " << transportPath << std::endl;
		return {-1, -1};
	}

	std::string subsysnqnPath = nvmeSysfsPath + "/subsysnqn";
	std::ifstream subsysnqnFile(subsysnqnPath);
	if (subsysnqnFile.is_open()) {
		std::string subsysnqn;
		std::getline(subsysnqnFile, subsysnqn);
		subsysnqnFile.close();

		int subsys_id = getSubsystemId(subsysnqn);
		return {subsys_id, ns_id};
	} else {
		return {-1, -1};
	}
}

MntpntMap::MntpntMap() {
    std::string dev_path, mnt_path;
    std::tie(dev_path, mnt_path) = getMountInfo();

    if (!dev_path.empty() && !mnt_path.empty()) {
        uint64_t dev_id = getDevId(dev_path);
        bool ret = Put(dev_id, dev_path);  
        if (!ret) {
            printf("[ERROR] failed to put\n");
        }
    }
}

bool MntpntMap::Put(const MntpntMapKeyType &dev_id, const std::string& dev_path) {
	auto mnt_path = findMntPath(dev_path);
	if (mnt_path.empty()) {
		return false;
	}

    std::string dev_name = dev_path.substr(5); // remove "/dev/"

    uint32_t subsys_id, ns_id;
    std::tie(subsys_id, ns_id) = getNvmeInfo(dev_name);
    /* TODO: print info*/
	if (subsys_id < 0 || ns_id < 0) {
		return false;
	} else {
		_map[dev_id] = std::make_tuple(mnt_path, subsys_id, ns_id);
        dmfp_metadata_info("mount device, dev_id=%u, dev_name=%s, path=%s\n", 
                dev_id, dev_name.c_str(), mnt_path.c_str());
		return true;
	}
}

std::vector<uint32_t> MntpntMap::GetDevIds() {
	std::vector<uint32_t> dev_ids;
	for (const auto& entry : _map) {
		dev_ids.push_back(entry.first);
	}
	return dev_ids;
}

std::string MntpntMap::GetMntpnt(const MntpntMapKeyType &dev_id) {
	auto it = _map.find(dev_id);
	if (it != _map.end()) {
		return std::get<0>(it->second);
	}
	return "";
}

std::pair<uint32_t, uint32_t> MntpntMap::Get2(const MntpntMapKeyType &dev_id) {
	auto it = _map.find(dev_id);
	if (it != _map.end()) {
		return {std::get<1>(it->second), std::get<2>(it->second)};
	}
	return {-1, -1};
}

void MntpntMap::Delete(const MntpntMapKeyType &dev_id) {
    _map.erase(dev_id);
}


bool FilepathMap::Put(const FilepathMapKeyType &key, const FilepathMapValueType &value) {
    auto it = _map.find(key);
    if (it == _map.end()) {
        _map[key] = value;
        return true;
    } else {
        it->second = value;
        return false;
    }
}

FilepathMapValueType FilepathMap::Get(const FilepathMapKeyType &key) {
	auto it = _map.find(key);
	if (it != _map.end()) {
		return it->second;
	}
	//return std::pair<uint16_t, std::string>();
	return {-1, ""};
}

void FilepathMap::Delete(const FilepathMapKeyType &key) {
    _map.erase(key);
}
