CC=gcc
CFLAGS=-W -Wall -pedantic -Os -O2
LDFLAGS=
EXEC=main

all: $(EXEC)

main: main.o
	$(CC) -o $(EXEC) $(EXEC).o $(LDFLAGS) -lz

clean:
	rm $(EXEC) *.o 

	
