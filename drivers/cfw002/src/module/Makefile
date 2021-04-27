KVER ?= $(shell uname -r)
KSRC=/lib/modules/$(KVER)/build

obj-m := cfw002.o


all: driver

test:
	echo "building for target $(KVER)"

help:
	$(MAKE) -C $(KSRC) M=$(PWD) help

driver:	
	$(MAKE) -C $(KSRC) M=$(PWD) modules

clean: 
	$(MAKE) -C $(KSRC) M=$(PWD) clean
	@rm -f Module.symvers
