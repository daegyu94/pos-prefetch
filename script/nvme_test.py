import subprocess
import time

file_path = "/mnt/nvme0/test.txt"
with open(file_path, 'r') as file:
    file_contents = file.read()

time.sleep(3)

# drop-cache.sh 실행
ret = subprocess.run(["/root/drop-cache.sh"])
print(ret)

time.sleep(3)

with open(file_path, 'r') as file:
    file_contents = file.read()
