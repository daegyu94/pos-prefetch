#pragma once

#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>

#include "my_types.h"

/* key=dev_id, value=[mnt_path, subsys_id, ns_id] */
using MntpntMapKeyType = uint32_t; 
using MntpntMapValueType = std::tuple<std::string, uint32_t, uint32_t>;

class MntpntMap {
public:
    MntpntMap();
    ~MntpntMap() {}

    void Initialize();
    bool Put(const MntpntMapKeyType &, const std::string &);
    std::vector<uint32_t> GetDevIds();
    std::string GetMntpnt(const MntpntMapKeyType &);
    std::pair<uint32_t, uint32_t> Get2(const MntpntMapKeyType &);
    void Delete(const MntpntMapKeyType &);

private:
    std::unordered_map<MntpntMapKeyType, MntpntMapValueType> _map;
};

/* key={dev_id, ino}, value=[filepath_type, filepath] */
using FilepathMapKeyType = DevIdInoPair;
using FilepathMapValueType = std::pair<uint16_t, std::string>;

class FilepathMap {
public:
    FilepathMap() {}
    ~FilepathMap() {}
    
    bool Put(const FilepathMapKeyType &, const FilepathMapValueType &); 
    FilepathMapValueType Get(const FilepathMapKeyType &); 
    void Delete(const FilepathMapKeyType &); 

private:
    std::unordered_map<FilepathMapKeyType, FilepathMapValueType> _map; 
};

extern class MntpntMap mntpnt_map;
extern class FilepathMap filepath_map;


static inline std::string findFilePathFast(DevIdInoPair &pair) {
    FilepathMapKeyType key = pair;
    auto ret = filepath_map.Get(key);
    auto path_type = ret.first;
    auto file_path = ret.second;

    if (path_type == 2) {
        auto mnt_path = mntpnt_map.GetMntpnt(pair.first);
        return mnt_path + file_path;
    } else {
        return file_path;
    }
}
