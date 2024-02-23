#include <iostream>
#include <algorithm>

#include "steering.h"

#define MAX_EXTENT_SIZE (128 * 1024)

#define DMFP_STEERING_INFO
#ifdef DMFP_TRANS_INFO
#define dmfp_steering_info(str, ...) \
    printf("[INFO] %s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_steering_info(str, ...) do {} while (0)
#endif

//#define DMFP_STEERING_DEBUG
#ifdef DMFP_TRANS_DEBUG
#define dmfp_steering_debug(str, ...) \
    printf("[DEBUG] %s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_steering_debug(str, ...) do {} while (0)
#endif

Steering steering;

static inline int EXTENT_ID(const uint64_t index) {
	return (index << PAGE_SHIFT) / MAX_EXTENT_SIZE;
}

struct ComparePageDeletionEvent {
	bool operator()(const PageDeletionEvent& a, const PageDeletionEvent& b) const {
        int a_extent_id = EXTENT_ID(a.index);
        int b_extent_id = EXTENT_ID(b.index);
		return std::tie(a.ino, a_extent_id) < std::tie(b.ino, b_extent_id);
	}
};

Steering::Steering() {
    dmfp_steering_info("");
}

std::vector<PageDeletionEvent> Steering::Sort(const std::vector<PageDeletionEvent>& input_list) {
    std::vector<PageDeletionEvent> sorted_list = input_list;
    std::sort(sorted_list.begin(), sorted_list.end(), ComparePageDeletionEvent());
    return sorted_list;
}

void Steering::Classify(const std::vector<PageDeletionEvent>& input_list) {
	uint64_t cur_inode = input_list[0].ino;
	uint64_t cur_extent_id = EXTENT_ID(input_list[0].index);
	int count = 1;

	for (size_t i = 1; i < input_list.size(); ++i) {
		uint64_t extent_id = EXTENT_ID(input_list[i].index);

		if (input_list[i].ino == cur_inode && extent_id == cur_extent_id) {
			count += 1;
		} else {
			bool adjacent = false;
			if (count > 1) {
				adjacent = true;
			}

			dict[std::make_pair(cur_inode, cur_extent_id)] = adjacent;

			cur_inode = input_list[i].ino;
			cur_extent_id = extent_id;
			count = 1;
		}
	}

	// Print the last element
	bool adjacent = false;
	if (count > 1) {
		adjacent = true;
	}

	dict[std::make_pair(cur_inode, cur_extent_id)] = adjacent;
}

std::vector<PageDeletionEvent> 
Steering::Filter(const std::vector<PageDeletionEvent>& input_list) {
	std::pair<uint64_t, uint64_t> used = std::make_pair(-1, -1);
	std::vector<PageDeletionEvent> ret_list;

	for (const auto& x : input_list) {
		uint64_t inode = x.ino;
		uint64_t extent_id = EXTENT_ID(x.index);

		if (dict[std::make_pair(inode, extent_id)]) {
			if (used == std::make_pair(inode, extent_id)) {
				continue;
			} else {
				if (used.first != -1) {
					dict.erase(used);
				}
				used = std::make_pair(inode, extent_id);
				ret_list.push_back(x);
			}
		} else {
			dict.erase(std::make_pair(inode, extent_id));
		}
	}

	if (used.first != -1) {
		dict.erase(used);
	}

	return ret_list;
}

#if 0
std::vector<std::vector<PageDeletionEvent>> 
Steering::FilterByInode(const std::vector<PageDeletionEvent>& input_list) {
	std::map<uint64_t, std::pair<uint64_t, uint64_t>> inode_dict;
	std::vector<std::vector<PageDeletionEvent>> ret_list;

	for (const auto& x : input_list) {
		uint64_t inode = x.ino;
		uint64_t extent_id = EXTENT_ID(x.index);

		if (dict[std::make_pair(inode, extent_id)]) {
			auto it = inode_dict.find(inode);
			if (it == inode_dict.end()) {
				inode_dict[inode] = std::make_pair(inode, extent_id);
				ret_list.push_back({x});
			} else {
				inode_dict[inode] = std::make_pair(inode, extent_id);
				ret_list.back().push_back(x);
			}
		} else {
			dict.erase(std::make_pair(inode, extent_id));
		}
	}

	return ret_list;
}
#endif 

std::vector<PageDeletionEvent> 
Steering::Process(const std::vector<PageDeletionEvent> &msg_list) {
    std::vector<PageDeletionEvent> sorted_list = Sort(msg_list);
	Classify(sorted_list);
	std::vector<PageDeletionEvent> ret_list = Filter(sorted_list);
	return ret_list;
}


#if 0
int main(void) {
    std::vector<PageDeletionEvent> vec;

    uint32_t dev_id = 100;
    uint64_t file_size = 1024;

    PageDeletionEvent event = { dev_id, 5, 3, file_size }; 
    vec.push_back(event);
    PageDeletionEvent event2 = { dev_id, 1, 1, file_size }; 
    vec.push_back(event2);
    PageDeletionEvent event3 = { dev_id, 4, 0, file_size }; 
    vec.push_back(event3);
    PageDeletionEvent event4 = { dev_id, 1, 2, file_size }; 
    vec.push_back(event4);
    PageDeletionEvent event5 = { dev_id, 5, 5, file_size }; 
    vec.push_back(event5);
    PageDeletionEvent event6 = { dev_id, 5, 4, file_size }; 
    vec.push_back(event6);
    
    printf("vec cap=%lu, size=%lu\n", vec.capacity(), vec.size());
    vec = steering.Process(vec);
    
    for (auto &e : vec) {
        printf("(%lu, %lu) ", e.ino, e.index);
    }
    printf("\n");

    vec.clear();
    printf("vec cap=%lu, size=%lu\n", vec.capacity(), vec.size());
}
#endif
