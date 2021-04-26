KVER=$(shell uname -r)
LIB_NAME=libcfw002
LIB_TARGET=$(LIB_NAME).so
LIB_OBJ=$(LIB_NAME).o
LIB_HEADER=$(LIB_NAME).h
LIB_SOURCE=$(LIB_NAME).c
API_HEADER=cfw002_api.h
API_INSTALL_PATH=/lib/modules/$(KVER)/iCubDrivers/cfw002/LinuxDriver/API

all: libcfw002.o
	ld -shared -soname $(LIB_TARGET) -o $(LIB_TARGET) -lc $(LIB_OBJ)

libcfw002.o: 
	gcc -O2 -fPIC -c $(LIB_SOURCE) -o $(LIB_OBJ)

clean:
	rm -f libcfw002.so
	rm -f *.a
	rm -f *.o

install: $(LIB_TARGET)
	# API
	mkdir -p $(API_INSTALL_PATH)
	cp $(LIB_HEADER) $(API_INSTALL_PATH)/
	cp $(API_HEADER) $(API_INSTALL_PATH)/
	cp $(LIB_TARGET) $(API_INSTALL_PATH)/
	ln -sf $(API_INSTALL_PATH)/$(LIB_TARGET) /usr/lib/$(LIB_TARGET)

uninstall:
	# API
	rm -rf $(API_INSTALL_PATH)	
	rm /usr/lib/$(LIB_TARGET)
