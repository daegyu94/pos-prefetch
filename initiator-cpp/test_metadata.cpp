#include <iostream>
#include <unordered_map>
#include <string>

#include "metadata.h"

using namespace std;

void test_mntpnt_map() {
    uint64_t dev_id = 271581185;

    mntpnt_map.GetDevIds();
    cout << mntpnt_map.GetMntpnt(dev_id) << endl;

    uint32_t subsys_id, ns_id;
    std::tie(subsys_id, ns_id) = mntpnt_map.GetPrefetchMeta(dev_id);

    cout << subsys_id << " " << ns_id << endl;
}

void test_filepath_map() {
    FilepathMapKeyType key = make_pair(0, 10);
    FilepathMapValueType value = make_pair(1, "/mnt/nvme0/a.txt");
    FilepathMapValueType ret;

    cout << value.first << ", " << value.second << endl;
    filepath_map.Put(key, value);
    
    ret = filepath_map.Get(key);
    //cout << ret.first << ", " << ret.second << endl;
}

int main() {
    
    test_mntpnt_map();

    test_filepath_map();

    return 0;
}
