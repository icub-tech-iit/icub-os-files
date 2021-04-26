#!/bin/bash -e
sudo mknod /dev/cfw002 c 280 0
sudo chmod 777 /dev/cfw002
sudo modprobe  firmware_class
sudo insmod /lib/modules/$(uname -r)/iCubDrivers/cfw002/LinuxDriver/cfw002.ko
exit 0
