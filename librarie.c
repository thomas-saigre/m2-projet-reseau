#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "stock.h"
#include "raler.h"

#define TITRE_S     10
#define ID_S        4
#define NB_S        2
#define MAXSOSK     32
#define MAXLEN      1024

void usage (char *argv0)
{
    fprintf (stderr, "usage: %s port livre1 libre2 ... livren\n", argv0) ;
    exit (1) ;
}




void traiter_commande(int s, struct stock *lib)
{
    // réception du datagramme
    struct sockaddr_storage sonadr ;
    socklen_t salong ;
    int r, af ;
    void *nadr ;			/* au format network */
    char padr [INET6_ADDRSTRLEN] ;	/* au format presentation */
    char buf [MAXLEN] ;

    salong = sizeof sonadr;
    r = recvfrom(s, buf, MAXLEN, 0, (struct sockaddr *) &sonadr, &salong);
    af = ((struct sockaddr *) &sonadr)->sa_family;

    switch (af)
    {
        case AF_INET:
            nadr = & ((struct sockaddr_in *) &sonadr)->sin_addr ;
            break ;
        case AF_INET6 :
            nadr = & ((struct sockaddr_in6 *) &sonadr)->sin6_addr ;
            break ;
    }
    inet_ntop(af, nadr, padr, sizeof padr);
    printf ("%s: nb d'octets lus = %d\n", padr, r);

    u_int32_t no_commande = *(u_int32_t *) buf;
    u_int16_t nb_livre = ntohs(*(u_int16_t *) &buf[4]);



    // Création du datagramme à renvoyer au serveur nil

    int ind = 6, n_dispo = 0, ind_livre;
    char dg_send[MAXLEN];
    memset(dg_send, 0, MAXLEN);

    // déjà en network byte order
    *(u_int32_t *) dg_send = no_commande;
    // on mettra le nombre de livres dans le dg à la fin

    for (int i=0; i<nb_livre; ++i)
    {
        char titre[TITRE_S + 1];
        memcpy(titre, &buf[ind], TITRE_S);
        titre[TITRE_S] = '\0';  // pour être sûr que ça se termine par \0
        ind_livre = est_disponible(titre, lib);

        if (ind_livre != -1)
        {
            printf("%s disponible !\n", &lib->livres[ind_livre * TITRE_S]);
            n_dispo++;
            snprintf(&dg_send[ind], TITRE_S,
                     "%s", &lib->livres[ind_livre * TITRE_S]);
        }
        else
        {
            printf("%s non disponible\n", titre);
        }
        
        ind += TITRE_S;
    }
    // on complmète le dg en ajoutant le nombre de livres
    *(uint16_t *) &dg_send[ID_S] = htons(n_dispo);


    // Envoi du datagramme
    int taille_dg = ID_S + NB_S + n_dispo*TITRE_S;
    r = sendto(s, dg_send, taille_dg, 0, (struct sockaddr *) &sonadr, salong);
    if (r == -1) raler(1, "sendto");
    printf("Réponse envoyée !\n\n");
}



int main(int argc, char *argv[])
{
    // gestion des arguments : mise en place du stock
    if (argc < 2) usage(argv[0]);
    
    int nb_livres = argc - 2;
    struct stock lib;
    init_stock(nb_livres, &argv[2], &lib);

    // ouverture du serveur UDP, en attente des envois de Nil
    int s[MAXSOSK], nsock, r;
    struct addrinfo hints, *res, *res0 ;
    char *cause;
    char *serv = argv[1];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC ;
    hints.ai_socktype = SOCK_DGRAM ;
    hints.ai_flags = AI_PASSIVE ;
    r = getaddrinfo (NULL, serv,  &hints, &res0) ;

    if (r != 0)
    {
        raler(0, "geteddrinfo: %s\n", gai_strerror(r));
    }

    // ouverture des sockets
    nsock = 0;
    for (res=res0; res && nsock<MAXSOSK; res=res->ai_next)
    {
        s[nsock] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s[nsock] == -1)
            cause = "socket";
        else
        {
            int o = 1;
            setsockopt(s[nsock], IPPROTO_IPV6, IPV6_V6ONLY, &o, sizeof o);

            r = bind(s[nsock], res->ai_addr, res->ai_addrlen);
            if (r == -1)
            {
                cause = "bind";
                close (s[nsock]);
            }
            else
                nsock++;
        }
    }
    if (nsock == 0) raler(1, cause);
    freeaddrinfo(res0);

    for (;;)
    {
        fd_set readfds;
        int i, max = 0;

        FD_ZERO(&readfds);
        for (i=0; i<nsock; ++i)
        {
            FD_SET(s[i], &readfds);
            if (s[i] > max)
                max = s[i];
        }

        if (select(max+1, &readfds, NULL, NULL, NULL) == -1)
            raler(0, "select");

        for (i=0; i<nsock; ++i)
        {
            if (FD_ISSET(s[i], &readfds))
                traiter_commande(s[i], &lib);
        }
    }

    free_stock(&lib);
    
    return 0;
}
