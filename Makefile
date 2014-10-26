LIBS=$(shell pkg-config --libs gtk+-3.0) -lpthread
CFLAGS=$(shell pkg-config --cflags gtk+-3.0) -Wall -Wextra -g
CC=gcc

OBJECTS=_obs/lightpager.o \
		_obs/fifoloop.o \
		_obs/config.o \
		_obs/timers.o

all:
	mkdir -p _obs && make lightpager

clean:
	rm -rf _obs

lightpager: $(OBJECTS)
	$(CC) $(LIBS) -o $@ $(OBJECTS)

_obs/lightpager.o: src/lightpager.c
	$(CC) $(CFLAGS) -c $^ -o $@
	
_obs/fifoloop.o: src/fifoloop.c
	$(CC) $(CFLAGS) -c $^ -o $@
	
_obs/config.o: src/config.c
	$(CC) $(CFLAGS) -c $^ -o $@

_obs/timers.o: src/timers.c
	$(CC) $(CFLAGS) -c $^ -o $@
