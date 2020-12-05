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
#include "commande.h"
#include <sys/time.h>

#define DISP

#define TITRE_S         10
#define ID_S            4
#define NB_S            2
#define REPONSE_S       29
#define MAXSOCK         64
#define MAXLEN          1024
#define IPv4            4
#define IPv6            6
#define IP_S            16

#define CHK(v) do{if((v)==-1)raler(1,#v);} while(0)

time_t delai;

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
 * Gère le retour des librairies pour les clients
 * 
 * @param s descripteur de la socket
 * @param rep addresse du répertoire
 */
void traiter_retour(int s, struct commande *cm)
{
    printf("\nRetour commande\n");
    struct sockaddr_storage sonadr ;
    socklen_t salong ;
    int r, af ;
    void *nadr;			        /* au format network */
    uint16_t *no_port;
    char padr [INET6_ADDRSTRLEN] ;	/* au format presentation */
    char buf [MAXLEN] ;
    char type;

    salong = sizeof sonadr ;
    r = recvfrom(s, buf, MAXLEN, 0, (struct sockaddr *) &sonadr, &salong) ;
    af = ((struct sockaddr *) &sonadr)->sa_family ;
    switch (af)
    {
    case AF_INET :
        nadr = & ((struct sockaddr_in *) &sonadr)->sin_addr ;
        type = IPv4;
        no_port = & ((struct sockaddr_in *) &sonadr)->sin_port ;
        break ;
    case AF_INET6 :
        nadr = & ((struct sockaddr_in6 *) &sonadr)->sin6_addr ;
        type = IPv6;
        no_port = & ((struct sockaddr_in6 *) &sonadr)->sin6_port ;
        break ;
    }

    char *nadr_;
    nadr_ = nadr;

    uint32_t no_commande = ntohs(*(uint32_t *) buf);
    uint16_t nb_livres = ntohs(*(uint16_t *) &buf[4]);

    int taille_dg = 2 + nb_livres * REPONSE_S;
    char *dg = malloc(taille_dg * sizeof(char));

    *(uint16_t *) dg = htons(nb_livres);
    int ind = 2;
    // int o;

    inet_ntop (af, nadr, padr, sizeof padr);
#ifdef DISP
    printf ("%s: nb d'octets lus = %d\n", padr, r);
#endif

    for (int i=0; i<nb_livres; ++i)
    {
        printf("Commande %d, traitement livre %d/%d\n", no_commande, i+1, nb_livres);
        // on recopie le titre du livre
        // for (o=0; o<TITRE_S; ++o)
        //     dg[ind + o] = buf[6 + i*TITRE_S + o];
        memcpy(&dg[ind], &buf[6 + i*TITRE_S], TITRE_S);
        // puis le type et l'adresse IP (v4 ou v6)
        dg[ind + TITRE_S] = type;
        switch (type)
        {
        case IPv4:
            memcpy(&dg[ind + TITRE_S + 1], nadr_, 4);
            memset(&dg[ind + TITRE_S + 5], 0, 12);
            break;
        case IPv6:
            printf("IPv6 : ");
            memcpy(&dg[ind + TITRE_S + 1], nadr_, IP_S);
            printf("\n");
            break;
        default:
            raler(0,"Normalement, ce message n'apparaitra jamais !");
            break;
        }
        // enfin le port TCP de la librairie
        
        *(uint16_t *) &dg[ind + TITRE_S + 1 + IP_S] = *no_port;
        // printf("%d %d", dg[ind + TITRE_S + 16 + 1], dg[ind + TI]);
        ind += REPONSE_S;
        
    }
    // même si on a 0 livre dans le retour, il faut mettre à jour la commande
    ajouter_commande(no_commande, nb_livres, dg,
            taille_dg, cm);
    CMD_DISP(cm);

}




/**
 * @brief gère l'envoi et la réception des données avec les librairies
 * 
 * @param an annuaire des librairies
 * @param dg datagramme à envoyer
 * @param len taille du datagramme
 * @param rep répertoire des commandes
 */
void broadcast_lib(const struct annuaire an, const char *dg, const int len)
{
    // client UDP : envoie aux librairies
    for (int l=0; l<an.nlib; ++l)
    {
        // pour l'envoi
        struct sockaddr_storage sadr;
        struct sockaddr_in  *sadr4 = (struct sockaddr_in  *) &sadr;
        struct sockaddr_in6 *sadr6 = (struct sockaddr_in6 *) &sadr;
        socklen_t salong;
        int r, family, o;

        // Envoi du datagramme à la librarie l
        memset(&sadr, 0, sizeof sadr);
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

        (void) family;



        o = 1;
        setsockopt(an.sock[l], SOL_SOCKET, SO_BROADCAST, &o, sizeof o);
        // setsockopt(s, SOL_SOCKET, SO_BROADCAST, &o, sizeof o);

#ifdef DISP
        printf("\t>Lib %d addr %s port %d\n", l, an.librairies[l], an.ports[l]);
#endif    
        r = sendto(an.sock[l], dg, len, 0, (struct sockaddr *) &sadr, salong);
        if (r == -1) raler(1, "send to");

    }
}





/**
 * @brief Prépare les données à envoyer aux librairies
 * 
 * @param in
 * @param no_commande numéro de commande
 * @param rep addresse du réperoitre des commandes
 * @param dg pointeur sur le datagramme à envoyer
 * @result taille du datagramme à envoyer
 */
int traiter_requete_client(int in, const uint32_t no_commande, char **dg)
{
    int r = 0;
    char buf[MAXLEN];

    // toutes les données sont contenus dans un unique dg de taille MAX_LEN max
    r = read(in, buf, MAXLEN);
    if (r == -1) raler(1, "read");
    syslog(LOG_ERR, "nb d'octers lus = %d\n", r);
    printf("\nrecu commande client %d, ", no_commande);
    printf("nb d'octers lus = %d\n", r);

    u_int16_t nb_livres = *(u_int16_t *) buf;
    u_int16_t n_l = ntohs(nb_livres);

    printf("Nb livres : %d\n", ntohs(nb_livres));

    int necr;
    // datagramme à envoyer à toutes les libraries
    int taille_dg = ID_S + NB_S + n_l*TITRE_S;
    printf("%d\n",taille_dg);
    *dg = calloc(taille_dg, sizeof(char));
    if (*dg == NULL)
        raler(0, "calloc");

    // numéro de commande
    *(uint32_t *) *dg = htons(no_commande);
    // nombre de livres
    *(u_int16_t *) &(*dg)[ID_S] = nb_livres;
    // titres des livres
    int ind = 6, ind_buf = 2;
    for (int i=0; i<n_l; ++i)
    {
        necr = snprintf(&(*dg)[ind], TITRE_S, "%s", &buf[ind_buf]);
        if (necr > TITRE_S)
            raler(0, "nil : Titre %s trop long", &buf[ind_buf]);
        ind += TITRE_S;
        ind_buf += TITRE_S;
    }

    return taille_dg;
}

/**
 * @brief Met en place le serveur TCP / UDP de nil
 * (TCP pour les clients, UDP pour les librairies)
 * 
 * @param serv
 * @param an annuaire des librairies
 */
void demon(char *serv, const struct annuaire an)
{
    int s[MAXSOCK], sd, nsock_tcp, r, opt = 1;
    struct addrinfo hints, *res, *res0;
    char *cause;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((r = getaddrinfo(NULL, serv, &hints, &res0)) != 0)
        raler(0, "getaddrinfo: %s\n", gai_strerror(r));


    // ouverture des sockets pour réception TCP des clients   
    nsock_tcp = 0;
    for (res = res0; res && nsock_tcp < MAXSOCK; res = res->ai_next)
    {
        s[nsock_tcp] = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        
        if (s[nsock_tcp] == -1)
            cause = "socket TCP";
        else
        {
            setsockopt(s[nsock_tcp], IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof opt);
            setsockopt(s[nsock_tcp], SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
            r = bind(s[nsock_tcp], res->ai_addr, res->ai_addrlen);

            if (r == -1)
            {
                cause = "bind TCP";
                close(s[nsock_tcp]);
            }
            else
            {
                listen(s[nsock_tcp], 5);
                nsock_tcp++;
            }
        }
    }
    if (nsock_tcp == 0) raler_log(cause);
    freeaddrinfo(res0);

    // ouverture des sockets UDP
    int nsock = nsock_tcp;
    for (int l=0; l < an.nlib; ++l)
    {
        // pour l'envoi
        struct sockaddr_storage sadr;
        struct sockaddr_in  *sadr4 = (struct sockaddr_in  *) &sadr;
        struct sockaddr_in6 *sadr6 = (struct sockaddr_in6 *) &sadr;
        socklen_t salong;
        int so, r, family, o;


        // Envoi du datagramme à la librarie l
        memset(&sadr, 0, sizeof sadr);
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

        so = socket(family, SOCK_DGRAM, 0);
        if (so == -1)
            raler(1, "socket %d", l);
        else
        {
            o = 1 ;		/* pour Linux */
            setsockopt(s[nsock], IPPROTO_IPV6, IPV6_V6ONLY, &o, sizeof o);
            (void) salong;
            (void) r;
            // r = bind(s[nsock], (struct sockaddr *) &sadr, salong);
            // if (r == -1)
            // {
            //     cause = "bind";
            //     close (s [nsock]);
            // }
            // else
            // {
                s[nsock] = so;
                nsock++;
                an.sock[l] = so;
            // }
        }
    }


    if (nsock == nsock_tcp) raler(1, cause);




    // indice de la dernière case occupée
    // int nb_sock = nsock_tcp;
    // s = [tcp0, tcp1, ..., tcpNt, udp0, udp1, ..., udpNu]

    uint32_t no_commande = 0;
    struct commande cm;
    init_commande(an.nlib, &cm);
    printf("Au début :\n");
    CMD_DISP(&cm);
    printf("\n");
    struct timeval attente = {0, 500000};   // 0.5 sec d'attente pour le select
    // attente d'une connexion d'un client
    for (;;)
    {
        fd_set readfds;
        int i, max = 0;
        FD_ZERO(&readfds);

        // set pour tous les descripteurs ouverts
        for (i=0; i<nsock_tcp; ++i)
        {
            FD_SET(s[i], &readfds);
            if (s[i] > max)
                max = s[i];
        }
        for (i=nsock_tcp; i<nsock; ++i)
        {
            FD_SET(s[i], &readfds);
            if (s[i] > max)
                max = s[i];
        }

        if (select(max+1, &readfds, NULL, NULL, &attente) == -1)
            raler_log("select");

        
        // requête d'un client
        for (i=0; i<nsock_tcp; ++i)
        {
            struct sockaddr_storage sonadr ;
            socklen_t salong ;

            if (FD_ISSET(s[i], &readfds))
            {
                no_commande++;
                salong = sizeof sonadr;
                sd = accept(s[i], (struct sockaddr *) &sonadr, &salong);

                nouvelle_commande(no_commande, sd, time(NULL) + delai, &cm);
                CMD_DISP(&cm);

                char *dg = NULL;
                int taille_dg;
                taille_dg = traiter_requete_client(sd, no_commande, &dg);

                broadcast_lib(an, dg, taille_dg);
                free(dg);
            }
        }

        // réponse d'une librairie
        for (i=nsock_tcp; i<nsock; ++i)
        {
            if (FD_ISSET(s[i], &readfds))
            {
                traiter_retour(s[i], &cm);
                CMD_DISP(&cm);
            }
        }

        // on regarde si un délai des commandes est arrivé à expiration
        tester_delai(&cm);

    }
}









int main(int argc, char *argv[])
{
    char *serv;
    if (argc < 3) usage(argv[0]);
    serv = argv[1];
    delai = (time_t) atoi(argv[2]);



    int narg = argc - 3;
    if (narg % 2 != 0)
    {
        printf("mauvais nombre d'arguments\n");
        usage(argv[0]);
    }
    int nlib = narg / 2;
    struct annuaire an;
    init_annuaire(nlib, &argv[3], &an);




    setsid();   // nouvelle session
    chdir("/"); // change le répertoire courant
    umask(0);   // initialise le masque binaire
    openlog("exemple", LOG_PID | LOG_CONS, LOG_DAEMON);
    demon(serv, an);
    exit(1);


    free_annuaire(&an);
    return 0;
}