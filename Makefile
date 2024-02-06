ifdef CROSS_COMPILE
	CC = $(CROSS_COMPILE)gcc
else 
	CC = gcc
endif

all: 
	${CC} finder-app/writer.c -o finder-app/writer

clean:
	rm -f finder-app/writer