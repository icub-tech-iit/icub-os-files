# CFW002
Holds sources for the cfw002 Linux device drivers.

## How to build
Just enter the `src` directory and type
`make all`
this will compie the module, the API, the firmware and the tests

If you want to compile only one of the above part, execute the make command in the corresponfind subdirectory.

## Installation
Use
`make install`
as superuser

The make script will install the following files:
* Firmware

`/lib/firmware/cfw002_fw.bin`

* Module

`/lib/modules/$(KVER)/iCubDrivers/cfw002/LinuxDriver/cfw002_fw.bin`

* API

`/lib/modules/$(KVER)/iCubDrivers/cfw002/LinuxDriver/API/cfw002_api.h
/lib/modules/$(KVER)/iCubDrivers/cfw002/LinuxDriver/API/libcfw002.h
/lib/modules/$(KVER)/iCubDrivers/cfw002/LinuxDriver/API/libcfw002.so
/usr/lib/libcfw002.so`

* Tests

`/lib/modules/$(KVER)/iCubDrivers/cfw002/LinuxDriver/tests/test_audio`

where

`KVER=$(shell uname -r)`
	
## Usage
load at starup the script S20_cfw002.sh

## Uninstallation
Use
`make uninstall`
as superuser
