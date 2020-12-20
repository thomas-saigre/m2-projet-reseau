#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <netdb.h>
#include "stock.h"
#include "raler.h"

#define TITRE_S     10
#define ID_S        4
#define NB_S        2
#define REP_S       11
#define MAXSOCK     32
#define MAXLEN      1024

#define CHK(v) do{if((v)==-1)raler(1,#v);} while(0)

void usage (char *argv0)
{
    fprintf(stderr, "usage: %s port livre1 libre2 ... livren\n", argv0);
    exit(1);
}

void raler_log (char *msg)
{
    syslog (LOG_ERR, "%s: %m", msg) ;
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
        printf(">> indice : %d\n", ind);
        char titre[TITRE_S + 1];
        memcpy(titre, &buf[6 + i * TITRE_S], TITRE_S);
        titre[TITRE_S] = '\0';  // pour être sûr que ça se termine par \0
        ind_livre = est_disponible(titre, lib);

        if (ind_livre != -1)
        {
            printf("%s disponible !\n", &lib->livres[ind_livre * TITRE_S]);
            n_dispo++;
            memcpy(&dg_send[ind], &lib->livres[ind_livre * TITRE_S], TITRE_S);

            ind += TITRE_S;
        }
        else
        {
            printf("%s non disponible\n", titre);
        }
    }
    // on complmète le dg en ajoutant le nombre de livres
    *(uint16_t *) &dg_send[ID_S] = htons(n_dispo);


    // Envoi du datagramme
    int taille_dg = ID_S + NB_S + n_dispo*TITRE_S;
    r = sendto(s, dg_send, taille_dg, 0, (struct sockaddr *) &sonadr, salong);
    if (r == -1) raler(1, "sendto");
    printf("Réponse envoyée ! (%d octets)\n\n", taille_dg);
}

/**
 * @brief Traite la réservation d'un client
 * 
 * @param s descripteur de la socket
 * @param lib addresse du stock
 */
void traiter_reservation(int s, struct stock *lib)
{
    char dg[MAXLEN], titre[TITRE_S + 1];
    int r;
    CHK(r = read(s, dg, MAXLEN));
    uint16_t nb_livresNBO = *(uint16_t *) dg;
    uint16_t nb_livres = ntohs(nb_livresNBO);
    int status;

    size_t taille_dg = nb_livres * REP_S + 2;
    char *dg_send = calloc(taille_dg + 2, sizeof(char));
    *(uint16_t *) dg_send = nb_livresNBO;

    int ind = 2;

    for (uint16_t l=0; l<nb_livres; ++l)
    {
        memcpy(titre, &dg[2 + l*TITRE_S], 10);
        memcpy(&dg_send[ind], &dg[2 + l*TITRE_S], 10);
        titre[TITRE_S] = '\0';
        status = reserver_livre(titre, lib);

        if (status == -1)   // libre non disponible
        {
            dg_send[ind + TITRE_S] = 0;
            printf("\t%s : non disponible\n", titre);
        }
        else
        {
            dg_send[ind + TITRE_S] = 1;
            printf("\t%s : réservé !\n", titre);
        }
        
        ind += REP_S;
    }

    CHK(r = write(s, dg_send, taille_dg));
    afficher_stock(lib);
}



int main(int argc, char *argv[])
{
    // gestion des arguments : mise en place du stock
    if (argc < 2) usage(argv[0]);
    
    int nb_livres = argc - 2;
    struct stock lib;
    init_stock(nb_livres, &argv[2], &lib);


    // ouverture du serveur TCP, en attente des envois des clients
    int s[MAXSOCK], nsock, nsock_tcp, r, sd, opt = 1;
    struct addrinfo hints, *res, *res0 ;
    char *cause;
    char *serv = argv[1];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((r = getaddrinfo(NULL, serv, &hints, &res0)) != 0)
        raler(0, "getaddrinfo: %s\n", gai_strerror(r));

    nsock = 0;
    for (res = res0; res && nsock < MAXSOCK; res = res->ai_next)
    {
        s[nsock] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        
        if (s[nsock] == -1)
            cause = "socket TCP";
        else
        {
            setsockopt(s[nsock], IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof opt);
            setsockopt(s[nsock], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
            r = bind(s[nsock], res->ai_addr, res->ai_addrlen);

            if (r == -1)
            {
                cause = "bind TCP";
                close(s[nsock]);
            }
            else
            {
                printf("TCP %d\n", s[nsock]);
                listen(s[nsock], 5);
                nsock++;
            }
        }
    }
    if (nsock == 0) raler_log(cause);
    freeaddrinfo(res0);

    nsock_tcp = nsock;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC ;
    hints.ai_socktype = SOCK_DGRAM ;
    hints.ai_flags = AI_PASSIVE ;
    r = getaddrinfo (NULL, serv,  &hints, &res0) ;
    if (r != 0)
        raler(0, "geteddrinfo: %s\n", gai_strerror(r));

    // ouverture des sockets UDP depuis nil
    for (res = res0; res && nsock < MAXSOCK; res = res->ai_next)
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
                // listen(s[nsock_tcp], 5);
                close (s[nsock]);
            }
            else
            {
                printf("UDP %d\n", s[nsock]);
                nsock++;
            }
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

        for (i = nsock_tcp; i < nsock; ++i)
        {
            if (FD_ISSET(s[i], &readfds))
                traiter_commande(s[i], &lib);
        }
        for (i = 0; i < nsock; ++i)
        {
            struct sockaddr_storage sonadr;
            socklen_t salong;
            void *nadr;
            uint16_t *no_port, port;
            int family;
            char padr[INET6_ADDRSTRLEN];
            if (FD_ISSET(s[i], &readfds))
            {
                salong = sizeof sonadr;
                sd = accept(s[i], (struct sockaddr *) &sonadr, &salong);

                family = ((struct sockaddr *) &sonadr)->sa_family ;

                switch (family)
                {
                case AF_INET:
                    nadr = &((struct sockaddr_in *) &sonadr)->sin_addr;
                    no_port = &((struct sockaddr_in *) &sonadr)->sin_port;
                    port = htons(*no_port);
                    break;
                case AF_INET6:
                    nadr = &((struct sockaddr_in6 *) &sonadr)->sin6_addr;
                    no_port = &((struct sockaddr_in6 *) &sonadr)->sin6_port;
                    port = htons(*no_port);
                    break;
                }
                // char *nadr_ = nadr;
                // uint16_t port = htons(*no_port);
                printf("%d\n", port);
                inet_ntop(family, nadr, padr, sizeof padr);
                printf("Done\n");
                printf("Commande de %s/\n", padr);
                (void) no_port;
                // printf("Commande de %s/%d\n", padr, *no_port);
                
                traiter_reservation(sd, &lib);
            }
        }
    }

    free_stock(&lib);
    
    return 0;
}
