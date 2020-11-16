# $ make


CC = gcc
CFLAGS = -c
DEBUG = -Wall -Werror -Wextra
SRC = librarie.c nil.c client.c
EXEC = $(SRC:.c=)

all: $(EXEC)

debug: DEBUG += -g
debug: all


librarie: librarie.o stock.o
	$(CC) librarie.o stock.o -o librarie

nil: nil.o
	$(CC) nil.o -o nil

client: client.o
	$(CC) client.o -o client



nil.o: nil.c
	$(CC) nil.c $(CFLAGS) $(DEBUG)

client.o: client.c
	$(CC) client.c $(CFLAGS) $(DEBUG)

librarie.o: librarie.c
	$(CC) librarie.c $(CFLAGS) $(DEBUG)


stock.o: stock.c
	$(CC) stock.c $(CFLAGS) $(DEBUG)


clean:
	rm -f *.o $(EXEC)
