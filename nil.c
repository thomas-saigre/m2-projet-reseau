#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_TITRE   10

void usage (char *argv0)
{
    fprintf (stderr, "usage: %s port délai librairie port ...\n", argv0) ;
    exit (1) ;
}


int main(int argc, char *argv[])
{
    printf("Coucou %d %s\n", argc, argv[0]);
    return 0;
}