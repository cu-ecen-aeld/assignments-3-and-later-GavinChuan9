CC := gcc
CROSS_COMPILE :=

ifdef CROSS_COMPILE
	CC := $(CROSS_COMPILE)gcc
endif

all: writer writer.o

writer : writer.o
	$(CC) -o writer writer.o

writer.o : writer.c
	$(CC) -c writer.c

clean:
	rm -f writer writer.o