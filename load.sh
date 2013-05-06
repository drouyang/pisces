#!/bin/sh

./linux_usr/mem_reserve 1
./linux_usr/pisces_load ../kitten-1.3.0/vmlwk.bin ../kitten-1.3.0/init_task "console=pisces"
./linux_usr/pisces_launch 
