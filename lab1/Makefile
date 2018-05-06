CC=clang-3.5
CFLAGS = -std=c99 -Wall -pedantic -g
SOURCE=main.c util.c #$(wildcard *.c)

all: pa1

pa1: $(SOURCE)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.log pa1
