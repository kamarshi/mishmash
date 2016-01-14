CC=gcc
CFLAGS=-I.


LIBS=-lm -lpthread

DEPS = gps_parser.h gps_buffer.h

OBJ = gps_parser.o gps_writer.o

APPS = gps_parse_main


%.o: %.c $(DEPS)
	$(CC)  -g -c -o $@ $< $(CFLAGS)

all: gps_parse_main

gps_parse_main: $(OBJ) gps_main.o
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o *~ core $(APPS)
