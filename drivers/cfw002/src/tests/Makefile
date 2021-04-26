KVER=$(shell uname -r)
TESTS_INSTALL_PATH=/lib/modules/$(KVER)/iCubDrivers/cfw002/LinuxDriver/tests

all: test_cfwlib test_filter test_audio test_notblock test_id_filter

test_cfwlib: test_cfwlib.c
	gcc test_cfwlib.c -O2 -o test_cfwlib -lcfw002           -I../API/ -L../API/
test_filter: test_filter.c
	gcc test_filter.c -O2 -o test_filter -lcfw002           -I../API/ -L../API/
test_audio: test_audio.c
	gcc test_audio.c -O2 -o test_audio -lcfw002             -I../API/ -L../API/
test_notblock: test_notblock.c
	gcc test_notblock.c -O2 -o test_notblock -lcfw002       -I../API/ -L../API/
test_id_filter: test_id_filter.c
	gcc test_id_filter.c -O2 -o test_id_filter -lcfw002     -I../API/ -L../API/

clean:
	rm -f test_cfwlib
	rm -f test_filter
	rm -f test_audio
	rm -f test_notblock
	rm -f test_id_filter

install: test_audio
	mkdir -p $(TESTS_INSTALL_PATH)
	cp test_audio $(TESTS_INSTALL_PATH)/

uninstall:
	-rm -rf $(TESTS_INSTALL_PATH)
