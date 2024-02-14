from collections import defaultdict
import inspect

MAX_EXTENT_SIZE = 128 * 1024
MIN_EXTENT_SIZE = 16 * 1024

steering_debug_on = False
def steering_debug(*args):
    if steering_debug_on:
        func_name = inspect.currentframe().f_back.f_code.co_name
        print("[DEBUG] Steering: {}: {}".format(func_name, " ".join(map(str, args))))

def EXTENT_ID(index):
    return (index << 12) // (MAX_EXTENT_SIZE)

class Steering:
    def __init__(self):
        print("[INFO] steering is on...")
        self.dict = defaultdict(None)
    
    """ Sorted by inode and by index """
    def sort(self, input_list):
        return sorted(input_list, key=lambda x: (x.ino, EXTENT_ID(x.index)))
    
    """ 
    If each inode is within a certain length,
    Returns the smallest page info.
    """
    def classify(self, input_list):
        cur_inode = input_list[0].ino
        cur_extent_id = EXTENT_ID(input_list[0].index)
        count = 1

        for i in range(1, len(input_list)):
            extent_id = EXTENT_ID(input_list[i].index)
            
            if input_list[i].ino == cur_inode and extent_id == cur_extent_id:
                count += 1
            else:
                adjacent = False
                if count > 1:
                    adjacent = True
                    #print(f"inode={cur_inode}, {cur_extent_id}S")
                else:
                    #print(f"inode={cur_inode}, {cur_extent_id}M")
                    pass

                self.dict[(cur_inode, cur_extent_id)] = adjacent

                cur_inode = input_list[i].ino
                cur_extent_id = extent_id
                count = 1

        # Print the last element
        adjacent = False
        if count > 1:
            adjacent = True
            #print(f"inode={cur_inode}, {cur_extent_id}S")
        else:
            #print(f"inode={cur_inode}, {cur_extent_id}M")
            pass

        self.dict[(cur_inode, cur_extent_id)] = adjacent

    def filter(self, input_list):
        used = (None, None)
        ret_list = []
        
        #print(self.dict)
        for x in input_list:
            inode = x.ino
            extent_id = EXTENT_ID(x.index)

            if self.dict[(inode, extent_id)] == True:
                if used == (inode, extent_id):
                    continue
                else:
                    if used[0] is not None:
                        del self.dict[used]
                    used = (inode, extent_id)
                    ret_list.append(x)
                    #print(f"{x} => Storage")
            else:
                del self.dict[(inode, extent_id)]
                #print(f"{x} => Memory")
        
        if used[0] is not None:
            del self.dict[used]
        
        return ret_list
    
    def filter_by_inode(self, input_list):
        inode_dict = {}
        ret_list = []

        for x in input_list:
            inode = x.ino
            extent_id = EXTENT_ID(x.index)

            if self.dict.get((inode, extent_id), False):
                if inode not in inode_dict:
                    inode_dict[inode] = (inode, extent_id)
                    ret_list.append([x])
                else:
                    inode_dict[inode] = (inode, extent_id)
                    ret_list[-1].append(x)
            else:
                del self.dict[(inode, extent_id)]
        return ret_list

    def process(self, msg_list):
        sorted_input_list = self.sort(msg_list)
        self.classify(sorted_input_list)
        ret_list = self.filter(sorted_input_list)
        
        steering_debug(ret_list)
        
        return ret_list
        

from my_types import *
if __name__ == "__main__":
    steering = Steering()
    
    dev_id = 0
    file_size = 0
    
    input_list = [
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 100, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 50, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 10, file_size),
            ]
    ret_list = steering.process(input_list)
    print("=== Storage: tc: same inode, discontiguous index  ===")
    print(ret_list)

    input_list = [
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 5, 1, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 2, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 4, 3, file_size),
            ]
    ret_list = steering.process(input_list)
    print("=== Storage: tc: different inode, adjacent index  ===")
    print(ret_list)

    input_list = [
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 5, 3, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 1, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 4, 0, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 2, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 5, 5, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 5, 4, file_size),
            ]
    ret_list = steering.process(input_list)
    print("=== Storage ===")
    print(ret_list)

    input_list = [
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 3, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 5, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 7, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 6, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 10, file_size),
            PageInfo(EventType.EVENT_PAGE_DELETION, dev_id, 1, 12, file_size),
            ]
    ret_list = steering.process(input_list)
    print("=== Storage ===")
    print(ret_list)
