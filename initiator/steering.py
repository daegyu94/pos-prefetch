from collections import defaultdict

from metadata import PageDeletionInfo
from config import *

def EXTENT_ID(file_offset):
    return file_offset // (ConstConfig.EXTENT_SIZE)

class Steering:
    def __init__(self):
        self.dict = defaultdict(None)
    
    def sort(self, input_list):
        return sorted(input_list, key=lambda x: (x.ino, EXTENT_ID(x.file_offset)))

    def classify(self, input_list):
        cur_inode = input_list[0].ino
        cur_extent_id = EXTENT_ID(input_list[0].file_offset)
        count = 1

        for i in range(1, len(input_list)):
            extent_id = EXTENT_ID(input_list[i].file_offset)
            
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
            extent_id = EXTENT_ID(x.file_offset)

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
   

    def process(self, msg_list):
        sorted_input_list = self.sort(msg_list)
        self.classify(sorted_input_list)
        
        return self.filter(sorted_input_list)
        

if __name__ == "__main__":
    steering = Steering()
    
    dev_id = 0
    file_size = 0
    
    input_list = [
            PageDeletionInfo(dev_id, 5, 1 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 1, 2 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 4, 3 * ConstConfig.EXTENT_SIZE, file_size),
            ]
    ret_list = steering.process(input_list)
    print("=== Storage ===")
    print(ret_list)

    input_list = [
            PageDeletionInfo(dev_id, 5, 1 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 1, 1 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 4, 1 * ConstConfig.EXTENT_SIZE, file_size),
            ]
    ret_list = steering.process(input_list)
    print("=== Storage ===")
    print(ret_list)

    input_list = [
            PageDeletionInfo(dev_id, 5, 3 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 1, 1 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 4, 0 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 1, 1 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 5, 3 * ConstConfig.EXTENT_SIZE, file_size),
            PageDeletionInfo(dev_id, 5, 4 * ConstConfig.EXTENT_SIZE, file_size),
            ]
    ret_list = steering.process(input_list)
    print("=== Storage ===")
    print(ret_list)

