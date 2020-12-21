# $ make


CC = gcc
CFLAGS = -c
DEBUG = -Wall -Werror -Wextra
SRC = librairie.c nil.c client.c
EXEC = $(SRC:.c=)

all: $(EXEC)

debug: DEBUG += -g
debug: all


librarie: librairie.o stock.o raler.o
	$(CC) librairie.o stock.o raler.o -o librarie

nil: nil.o raler.o annuaire.o commande.o
	$(CC) nil.o raler.o annuaire.o commande.o -o nil

client: client.o raler.o retour.o
	$(CC) client.o raler.o retour.o -o client



nil.o: nil.c
	$(CC) nil.c $(CFLAGS) $(DEBUG)

annuaire.o: annuaire.c
	$(CC) annuaire.c $(CFLAGS) $(DEBUG)

commande.o: commande.c
	$(CC) commande.c $(CFLAGS) $(DEBUG)

client.o: client.c
	$(CC) client.c $(CFLAGS) $(DEBUG)

librairie.o: librairie.c
	$(CC) librairie.c $(CFLAGS) $(DEBUG)

raler.o: raler.c
	$(CC) raler.c $(CFLAGS) $(DEBUG)

retour.o: retour.c
	$(CC) retour.c $(CFLAGS) $(DEBUG)

stock.o: stock.c
	$(CC) stock.c $(CFLAGS) $(DEBUG)


clean:
	rm -f *.o $(EXEC)
