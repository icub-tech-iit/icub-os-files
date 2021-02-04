#!/bin/bash -e
# Set iCub speakers volume - to use only if iCub has cfw002
/lib/modules/$(uname -r)/iCubDrivers/cfw002/LinuxDriver/tests/test_audio 6
exit 0
