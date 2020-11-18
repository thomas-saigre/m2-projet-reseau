# $ make


CC = gcc
CFLAGS = -c
DEBUG = -Wall -Werror -Wextra
SRC = librarie.c nil.c client.c
EXEC = $(SRC:.c=)

all: $(EXEC)

debug: DEBUG += -g
debug: all


librarie: librarie.o stock.o raler.o
	$(CC) librarie.o stock.o raler.o -o librarie

nil: nil.o raler.o annuaire.o
	$(CC) nil.o raler.o annuaire.o -o nil

client: client.o raler.o
	$(CC) client.o raler.o -o client



nil.o: nil.c
	$(CC) nil.c $(CFLAGS) $(DEBUG)

annuaire.o: annuaire.c
	$(CC) annuaire.c $(CFLAGS) $(DEBUG)

client.o: client.c
	$(CC) client.c $(CFLAGS) $(DEBUG)

librarie.o: librarie.c
	$(CC) librarie.c $(CFLAGS) $(DEBUG)

raler.o: raler.c
	$(CC) raler.c $(CFLAGS) $(DEBUG)

stock.o: stock.c
	$(CC) stock.c $(CFLAGS) $(DEBUG)


clean:
	rm -f *.o $(EXEC)
