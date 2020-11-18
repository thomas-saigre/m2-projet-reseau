#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "raler.h"

#define TITRE_S   10

void usage (char *argv0)
{
    fprintf(stderr, "usage: %s serveur port livre1 libre2 ... livren\n", argv0);
    exit (1) ;
}

/**
 * @brief envoie une requête TCP au serveur NIL
 * @param serveur addresse sur serveur (IPv4 ou IPv6)
 * @param port port TCP du serveur
 * @param n nombre de livre à commander
 * @param livre référence de l'ouvrage
 */
void envoyer_requete(const char *serveur, const char *port,
    const uint16_t n, char *livre[])
{
    int err;

    const int taile_dg = 2 + n*TITRE_S;
    char *datagramme = calloc(taile_dg, sizeof(char));

    *(uint16_t *) datagramme = n;
    int i_dg = 2;
    for (int i=0; i<n; ++i)
    {
        int n_ecrits = snprintf(&datagramme[i_dg], 10, "%s", livre[i]);
        if (n_ecrits > 10) raler(0, "Titre %s trop long", livre[i]);
        i_dg += TITRE_S;
    }

    struct addrinfo hints, *res, *res0 ;
    hints.ai_family = PF_UNSPEC ;
    hints.ai_socktype = SOCK_STREAM ;

    int r = getaddrinfo(serveur, port, &hints, &res0);
    if (r != 0) raler(0, "getaddrinfo: %s\n", gai_strerror(r));

    int s = -1;
    char *cause;
    for (res = res0; res != NULL; res = res->ai_next)
    {
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s == -1) cause = "socket";
        else
        {
            r = connect(s, res->ai_addr, res->ai_addrlen);
            if (r == -1)
            {
                cause = "connect";
                close(s);
                s = -1;
            }
            else break;
        }
    }
    if (s == -1) raler(0, "Erreur : %s", cause);
    freeaddrinfo(res0);
    
    printf("Envoi requête à %s/%s", serveur, port);
    err = write(s, datagramme, taile_dg);
    if (err == -1) raler(1, "Échec envoi requête");

    close(s);

    free(datagramme);
}



int main(int argc, char *argv[])
{
    if (argc < 3) usage(argv[0]);

    int nb_livres = argc - 3;

    envoyer_requete(argv[1], argv[2], nb_livres, &argv[3]);

    return 0;
}