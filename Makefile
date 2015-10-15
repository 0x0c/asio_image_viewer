CC = gcc
CFLAGS = -Wall -I/opt/local/include -lstdc++ `pkg-config --cflags opencv` `pkg-config --libs opencv`
LDLIBS = /opt/local/lib/libboost_system-mt.dylib


all:
	$(CC) main.cpp $(LDLIBS) $(CFLAGS)

run:
	./a.out
	
clean:
	rm ./a.out