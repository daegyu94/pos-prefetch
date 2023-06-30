import random
import string
import subprocess
import time
  
file_path = '/mnt/nvme/test.txt'
block_size = 4 * 1024
num_blocks = 10
random.seed(0)

def generate_random_string(size):
    return ''.join(random.choice(string.ascii_lowercase) for _ in range(size))

def compare_content(i, str1, str2):
    if str1 != str2:
        print(f"Err: Blk{i}: not matched")
    else:
        print(f"Ok: Blk{i}: matched")

def write_list_to_file(file_path, string_list):
    with open(file_path, 'w') as file:
        for item in string_list:
            file.write(item)

def read_file_to_list(file_path, string_list, drop):
    with open(file_path, 'r') as file:
        i = 0
        while True:
            content = file.read(block_size)
            if not content:
                break
            compare_content(i, content, string_list[i])
            i += 1

    print('read done')
    if drop:
        ret = subprocess.run(["/home/daegyu/drop-cache.sh"])
        print(ret)

if __name__ == "__main__":
    string_list = [generate_random_string(block_size) for _ in range(num_blocks)]
    """
    string_list = []
    for _ in range(num_blocks):
        random_string = generate_random_string(block_size)
        string_list.append(random_string)
    """
    write_list_to_file(file_path, string_list)


    read_string_list = read_file_to_list(file_path, string_list, True)
    time.sleep(3)
    
    read_string_list = read_file_to_list(file_path, string_list, False)
