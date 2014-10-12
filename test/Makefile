#CC := arm-none-linux-gnueabi-gcc
#LD := arm-none-linux-gnueabi-gcc
CC := gcc
LD := gcc
CFLAGS := -g -Wall -Werror #-static
# LDFLAGS := -static

OBJECTS := AccTest.o

all: AccTest


: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f AccTest
	rm -f AccTest.o

.PHONY: clean
