#pragma once

#include <vector>
#include <map>

#include "my_types.h"

class Steering {
public:
    Steering();

    std::vector<PageDeletionEvent> Process(const std::vector<PageDeletionEvent>& msg_list);

private:
    std::vector<PageDeletionEvent> Sort(const std::vector<PageDeletionEvent>& input_list);
    void Classify(const std::vector<PageDeletionEvent>& input_list);
    std::vector<PageDeletionEvent> Filter(const std::vector<PageDeletionEvent>& input_list);
    std::vector<std::vector<PageDeletionEvent>> 
        FilterByInode(const std::vector<PageDeletionEvent>& input_list);

    std::map<std::pair<uint64_t, uint64_t>, bool> dict;
};

extern Steering steering;
