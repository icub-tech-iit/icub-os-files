KVER ?= $(shell uname -r)
KSRC=/lib/modules/$(KVER)/build
INSTALL_PREFIX ?= 
MODULE_NAME = cfw002

obj-m := $(MODULE_NAME).o


all: driver

help:
	$(MAKE) -C $(KSRC) M=$(PWD) help

driver:	
	$(MAKE) -C $(KSRC) M=$(PWD) modules

clean: 
	$(MAKE) -C $(KSRC) M=$(PWD) clean
	@rm -f Module.symvers

install: $(MODULE_NAME).ko
	mkdir -p $(INSTALL_PREFIX)/lib/modules/${KVER}/iCubDrivers/cfw002/LinuxDriver/
	cp $(MODULE_NAME).ko $(INSTALL_PREFIX)/lib/modules/${KVER}/iCubDrivers/cfw002/LinuxDriver/

uninstall:
	rm $(INSTALL_PREFIX)/lib/modules/${KVER}/iCubDrivers/cfw002/LinuxDriver/$(MODULE_NAME).ko
