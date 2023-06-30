#!/bin/bash 

sudo mkfs.ext4 /dev/nvme0n1
sudo mount /dev/nvme0n1 /mnt/nvme

sudo python3 content_comparison.py

sleep 1

sudo umount -l /mnt/nvme
