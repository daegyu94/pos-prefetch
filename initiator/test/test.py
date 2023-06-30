import subprocess

mnt_path="/mnt/nvme0"
#ino=1835025
ino=18350111

cmd = f"find {mnt_path} -xdev -inum {ino}"
output = subprocess.check_output(cmd, shell=True, text=True)
if (output):
    print(output.split()[0])


import os

pid = os.getpid()

niceness = 10  # 변경할 nice 값. 높은 숫자일수록 우선순위가 낮아집니다.

os.nice(niceness)

new_niceness = os.nice(0)
print("New nice value:", new_niceness)
