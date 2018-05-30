CC=clang-3.5
CFLAGS = -std=c99 -Wall -pedantic -g -L. -lruntime
SOURCE=$(wildcard *.c)

all: pa4

pa4: $(SOURCE)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.log pa4
