#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include "raler.h"
#include "annuaire.h"

#define TITRE_S 10
#define ID_S    4
#define NB_S    2
#define MAXSOCK 32
#define MAXLEN  1024

void usage (char *argv0)
{
    fprintf (stderr, "usage: %s port délai librairie port ...\n", argv0) ;
    exit (1) ;
}

void raler_log (char *msg)
{
    syslog (LOG_ERR, "%s: %m", msg) ;
    exit (1) ;
}


/**
 * @brief Envoie le datagramme de la commande à toutes les libraries
 * 
 * @param an structure contenant les adresses et les ports des librairies
 * @param dg datagramme à envoyer
 * @param n taille du datagramme
 */
void brodcast_command(const struct annuaire an, const char *dg, const int len)
{
    for (int l=0; l<an.nlib; ++l)
    {
        struct sockaddr_storage sadr;
        struct sockaddr_in  *sadr4 = (struct sockaddr_in  *) &sadr;
        struct sockaddr_in6 *sadr6 = (struct sockaddr_in6 *) &sadr;
        socklen_t salong;
        int s, r, family, o;

        memset (&sadr, 0, sizeof sadr);
        int port = htons(an.ports[l]);

        if (inet_pton(AF_INET6, an.librairies[l], &sadr6->sin6_addr) == 1)
        {
            family = PF_INET6;
            sadr6->sin6_family = AF_INET6;
            sadr6->sin6_port = port;
            salong = sizeof *sadr6;
        }
        else if (inet_pton(AF_INET6, an.librairies[l], &sadr4->sin_addr) == 1)
        {
            family = PF_INET ;
            sadr4->sin_family = AF_INET;
            sadr4->sin_port = port;
            salong = sizeof *sadr4;
        }
        else
            raler(0, "adresse '%s' non reconnue\n", an.librairies[l]);

        s = socket(family, SOCK_DGRAM, 0);
        if (s==-1) raler(0, "socket");

        o = 1;
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, &o, sizeof o);

        printf("Envoi à la librairie %d\n", l);
        r = sendto(s, dg, len, 0, (struct sockaddr *) &sadr, salong);
        if (r == -1) raler(1, "send to");

        close(s);
    }
}






void serveur(int in, const uint32_t no_commande, const struct annuaire an)
{
    int r, n = 0;
    char buf[MAXLEN];


    while ((r = read(in, buf, MAXLEN)) > 0)
        n += r;
    syslog(LOG_ERR, "nb d'octers lus = %d\n", n);
    printf("nb d'octers lus = %d\n", n);

    u_int16_t nb_livres = *(u_int16_t *) buf;
    u_int16_t n_l = ntohs(nb_livres);

    printf("Nb livres : %d\n", ntohs(nb_livres));

    int necr;
    // datagramme à envoyer à toutes les libraries
    int taille_dg = ID_S + NB_S + n_l*TITRE_S;
    printf("%d\n",taille_dg);
    char *dg = malloc(taille_dg*sizeof(char));

    // numéro de commande
    *(uint32_t *) dg = htons(no_commande);
    // nombre de livres
    *(u_int16_t *) &dg[ID_S] = nb_livres;
    // titres des livres
    int ind = 6, ind_buf = 2;
    for (int i=0; i<n_l; ++i)
    {
        necr = snprintf(&dg[ind], TITRE_S, "%s", &buf[ind_buf]);
        if (necr > TITRE_S)
            raler(0, "nil : Titre %s trop long", &buf[ind_buf]);
        ind += TITRE_S;
        ind_buf += TITRE_S;
    }

    brodcast_command(an, dg, taille_dg);



}


void demon(char *serv, const struct annuaire an)
{
    int s[MAXSOCK], sd, nsock, r, opt = 1;
    struct addrinfo hints, *res, *res0;
    char *cause;

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
            cause = "socket";
        else
        {
            setsockopt(s[nsock], IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof opt);
            setsockopt(s[nsock], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
            r = bind(s[nsock], res->ai_addr, res->ai_addrlen);

            if (r == -1)
            {
                cause = "bind";
                close(s[nsock]);
            }
            else
            {
                listen(s[nsock], 5);
                nsock++;
            }
        }
    }

    if (nsock == 0) raler_log(cause);
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
            raler_log("select");

        uint32_t no_commande = 0;

        for (i=0; i<nsock; ++i)
        {
            struct sockaddr_storage sonadr ;
            socklen_t salong ;

            if (FD_ISSET(s[i], &readfds))
            {
                no_commande++;
                salong = sizeof sonadr;
                sd = accept(s[i], (struct sockaddr *) &sonadr, &salong);
                switch (fork())
                {
                case -1:
                    raler(1, "fork");
                    break;
                case 0:
                    serveur(sd, no_commande, an);
                    exit(0);
                default:
                    close(sd);
                    break;
                }
            }
        }
    }
}





int main(int argc, char *argv[])
{
    char *serv;
    if (argc < 3) usage(argv[0]);
    serv = argv[1];
    (void) serv;



    int narg = argc - 3;
    if (narg % 2 != 0)
    {
        printf("mauvais nombre d'arguments\n");
        usage(argv[0]);
    }
    int nlib = narg / 2;
    struct annuaire an;
    init_annuaire(nlib, &argv[3], &an);


    // for (int i=0; i<an.nlib; ++i)
    //     printf("%s : %d\n", an.librairies[i], an.ports[i]);

    // char *dg;
    // dg = "thomas";
    // brodcast_command(an, dg, 6);

    setsid();   // nouvelle session
    chdir("/"); // change le répertoir courant
    umask(0);   // initialise le masque binaire
    openlog("exemple", LOG_PID | LOG_CONS, LOG_DAEMON);
    demon(serv, an);
    exit(1);


    free_annuaire(&an);
    return 0;
}