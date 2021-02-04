#!/bin/bash -e
mknod /dev/cfw002 c 280 0
chmod 777 /dev/cfw002
modprobe  firmware_class
insmod /lib/modules/$(uname -r)/iCubDrivers/cfw002/LinuxDriver/cfw002.ko
exit 0
