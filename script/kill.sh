#!/bin/bash 

pid=$(pidof pos_prefetch.out)
sudo kill -9 $pid
