all:
	cd firmware && $(MAKE)
	cd API && $(MAKE)
	cd module && $(MAKE)
	cd tests && $(MAKE)

clean:
	cd firmware && $(MAKE) clean
	cd module && $(MAKE) clean
	cd API && $(MAKE) clean
	cd tests && $(MAKE) clean

install:
	cd firmware && $(MAKE) install
	cd API && $(MAKE) install
	cd module && $(MAKE) install
	cd tests && $(MAKE) install

uninstall:
	cd firmware && $(MAKE) uninstall
	cd API && $(MAKE) uninstall
	cd module && $(MAKE) uninstall
	cd tests && $(MAKE) uninstall
