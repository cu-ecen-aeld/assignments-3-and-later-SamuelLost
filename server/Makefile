ifdef CROSS_COMPILE
	CC = $(CROSS_COMPILE)gcc
else 
	CC = gcc
endif

all: aesdsocket
aesdsocket: aesdsocket.c
	$(CC) -Wall -Werror aesdsocket.c -o aesdsocket

.PHONY: clean
clean:
	rm -f aesdsocket