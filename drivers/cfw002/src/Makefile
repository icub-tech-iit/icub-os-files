all:
	cd firmware && $(MAKE) all
	cd API && $(MAKE) all
	cd module && $(MAKE) all
	cd tests && $(MAKE) all

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
	cd load_script && $(MAKE) install

uninstall:
	cd firmware && $(MAKE) uninstall
	cd API && $(MAKE) uninstall
	cd module && $(MAKE) uninstall
	cd tests && $(MAKE) uninstall
	cd load_script && $(MAKE) uninstall
