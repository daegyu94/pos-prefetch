#!/bin/bash 

dev_path="/dev/nvme0n1"
mnt_path="/mnt/nvme0"

sudo mkfs.ext4 /dev/nvme0n1
sudo mount /dev/nvme0n1 /mnt/nvme0
sudo dd if=/dev/zero of=/mnt/nvme0/test.txt bs=1M count=1
sync;

sudo python3 nvme_test.py 

sudo umount -l /mnt/nvme0
