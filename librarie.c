#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "stock.h"

#define TITRE_S   10

void usage (char *argv0)
{
    fprintf (stderr, "usage: %s port livre1 libre2 ... livren\n", argv0) ;
    exit (1) ;
}







int main(int argc, char *argv[])
{
    if (argc < 2) usage(argv[0]);
    
    int nb_livres = argc - 2;
    
    struct stock lib;
    init_stock(nb_livres, &argv[2], &lib);
    
    
    free_stock(&lib);
    
    return 0;
}
