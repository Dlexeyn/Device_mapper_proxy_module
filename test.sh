#!/bin/bash
size=200

make

sudo insmod dmp.ko

sudo dmsetup create zero1 --table "0 $size zero"

sudo dmsetup create dmp1 --table "0 $size dmp /dev/mapper/zero1"

ls -al /dev/mapper/*

sudo dd if=/dev/random of=/dev/mapper/dmp1 bs=4k count=1

sudo dd of=/dev/null if=/dev/mapper/dmp1 bs=4k count=1

cat /sys/module/dmp/stat/volumes