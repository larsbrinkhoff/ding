all: ding

CFLAGS=-W -Wall
LDLIBS=-lhidapi-libusb

ding: ding.o

clean:
	rm -f ding *.o
