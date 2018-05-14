CC=clang-3.5
CFLAGS = -std=c99 -Wall -pedantic -g -L. -lruntime
SOURCE=$(wildcard *.c)

all: pa2

pa2: $(SOURCE)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.log pa2
