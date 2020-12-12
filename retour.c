#include "retour.h"
#include "raler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#define IPv4    4
#define IPv6    6
#define IPv4LEN 32
#define IPv6LEN 128

#define CHK(v) do{if((v)==-1)raler(1,#v);} while(0)

void init_retour(const int n, struct retour *ret)
{
    ret->n = n;
    ret->current_ind = 0;
    ret->nlib = INIT_LIB;

    ret->ind = malloc(n * sizeof(int));
    if (ret->ind == NULL) raler(0, "Malloc");
    memset(ret->ind, -1, n * sizeof(int));

    ret->titre = malloc(n * sizeof(char *));
    if (ret->titre == NULL) raler(0, "Malloc");
    for (int i=0; i<n; ++i)
        ret->titre[i] = "";

    ret->datagrammes = malloc(INIT_LIB * sizeof(char *));
    if (ret->datagrammes == NULL) raler(0, "Malloc");
    memset(ret->datagrammes, 0, INIT_LIB * sizeof(char *));

    ret->taille_dg = malloc(INIT_LIB * sizeof(int));
    if (ret->taille_dg == NULL) raler(0, "malloc");
    memset(ret->taille_dg, 0, INIT_LIB*sizeof(int));
    
    ret->type = malloc(INIT_LIB * sizeof(uint8_t));
    if (ret->type == NULL) raler(0, "Malloc");
    
    ret->port = malloc(INIT_LIB * sizeof(uint16_t));
    if (ret->port == NULL) raler(0, "Malloc");
    memset(ret->port, 0, INIT_LIB * sizeof(uint16_t));
    
    ret->Ip = malloc(INIT_LIB * sizeof(char *));
    if (ret->Ip == NULL) raler(0, "Malloc");
    for (int i=0; i<INIT_LIB; ++i)
    {
        ret->Ip[i] = malloc(IP_S * sizeof(char));
        if (ret->Ip[i] == NULL) raler(0, "Malloc");
    }
}

void disp(const struct retour *ret)
{
    printf("Livres (%d)\n", ret->n);
    for (int i=0; i<ret->n; ++i)
    {
        printf("%d : %d '%s'\n", i, ret->ind[i], ret->titre[i]);
    }
    printf("\nLibrairies (%d)\n", ret->nlib);
    for (int i = 0; i < ret->nlib; ++i)
    {
        printf("%d : %p %d %d -", i, ret->datagrammes[i], ret->type[i], ret->port[i]);
        for (int k=0; k<16; ++k)
            printf("%d ", ret->Ip[i][k]);
        printf("\n");
    }
}

void free_retour(struct retour *ret)
{
    free(ret->ind);
    free(ret->titre);
    // on libère la mémoire des datagrammes au moment de l'envoi
    free(ret->datagrammes);
    free(ret->type);
    free(ret->port);
    free(ret->Ip);
}

int recherche_librairie(const char* addr,const uint16_t port,const uint8_t type,
        struct retour *ret)
{
    int ind = 0;
    int ind_libre, is_libre = 0;

    // on recherche si on a déjà commencé un dg pour la librairie
    for (; ind<ret->nlib; ++ind)
    {
        if ((memcmp(addr, ret->Ip[ind], IP_S) == 0) &&
             port == ret->port[ind])
            break;
        if ((ret->port[ind] == 0) && (is_libre == 0))
        {
            is_libre = 1;
            ind_libre = ind;
        }
    }

    if (ind < ret->nlib)    // on a trouvé la librairie
    {
        return ind;
    }
    else
    {
        if (is_libre == 1)  // une place est libre
        {
            memcpy(ret->Ip[ind_libre], addr, IP_S);
            ret->port[ind_libre] = port;
            ret->type[ind_libre] = type;
            return ind_libre;
        }
        else                // sinon on rajoute une case
        {
            ret->nlib += 1;

            ret->datagrammes = realloc(ret->Ip, ret->nlib * sizeof(char *));
            if (ret->datagrammes == NULL) raler(0, "realloc");
            ret->datagrammes[ret->nlib-1] = NULL;

            ret->Ip = realloc(ret->Ip, ret->nlib * sizeof(char *));
            if (ret->Ip == NULL) raler(0, "realloc");
            memcpy(ret->Ip[ret->nlib-1], addr, IP_S);

            ret->port = realloc(ret->port, ret->nlib * sizeof(uint16_t));
            if (ret->port == NULL) raler(0, "realloc");

            ret->port[ret->nlib-1] = port;
            
            ret->type = realloc(ret->type, ret->nlib * sizeof(uint8_t));
            if (ret->type == NULL) raler(0, "realloc");
            ret->type[ret->nlib-1] = type;

            return ret->nlib - 1;
        }
        
    }
    
}

void ajouter_livre(char *titre, const int ind, const int ind_lib,
         struct retour *ret)
{
    ret->ind[ret->current_ind] = ind;
    ret->titre[ret->current_ind] = titre;

    if (ret->taille_dg[ind_lib] == 0)   // on n'a pas encore commencé à écrire le dg
    {
        ret->datagrammes[ind_lib] = malloc(2 + TITRE_S);
        memset(ret->datagrammes[ind_lib], 0, 2+TITRE_S);
        if (ret->datagrammes[ind_lib] == NULL)
            raler(0, "malloc");
        
        *(uint16_t *) ret->datagrammes[ind_lib] = htons(1);
        strncpy(&ret->datagrammes[ind_lib][2], titre, TITRE_S);

        ret->taille_dg[ind_lib] = 2 + TITRE_S;
    }
    else
    {
        ret->datagrammes[ind_lib] = realloc(ret->datagrammes[ind_lib],
                ret->taille_dg[ind_lib] + TITRE_S);
        if (ret->datagrammes[ind_lib] == NULL)
            raler(0, "realloc");
        memset(&ret->datagrammes[ind_lib][ret->taille_dg[ind_lib]], 0, TITRE_S);

        uint16_t old_nb_liv = ntohs(*(uint16_t *) ret->datagrammes[ind_lib]);
        *(uint16_t *) ret->datagrammes[ind_lib] = htons(old_nb_liv + 1);

        int ind_tmp = (ret->taille_dg[ind_lib] - 2) / TITRE_S;
        strncpy(&ret->datagrammes[ind_lib][ind_tmp], titre, TITRE_S);

        ret->taille_dg[ind_lib] += TITRE_S;
    }
    
}

int rechercher_livre(const char *titre, struct retour *ret)
{
    int ind = -1;
    for (int i=0; i<ret->n; ++i)
    {
        if (strncmp(titre, ret->titre[i], TITRE_S) == 0)
        {
            ind = i;
            break;
        }
    }
    return ind;
}

void envoyer_dg(fd_set *fd, int *max, int so[MAXSOCK], int *nsock,
        struct retour *ret)
{
    // on ouvre une socket par librairie
    for (int l=0; l<ret->nlib; ++l)
    {
        struct sockaddr_storage ladr;
        memset(&ladr, 0, sizeof(ladr));
        struct sockaddr_in *ladr4 = (struct sockaddr_in *)&ladr;
        struct sockaddr_in6 *ladr6 = (struct sockaddr_in6 *)&ladr;
        int s, family, r, err;
        socklen_t llong;
        printf("%d\n", ret->type[l]);
        switch (ret->type[l])
        {
        case IPv4:
            memcpy(&ladr4->sin_addr, ret->Ip[l], IPv4LEN);
            family = AF_INET;
            ladr4->sin_family = AF_INET;
            ladr4->sin_port = ret->port[l];
            llong = sizeof(*ladr4);
            break;
        case IPv6:
            memcpy(&ladr6->sin6_addr, ret->Ip[l], IPv6LEN);
            family = AF_INET6;
            ladr6->sin6_family = AF_INET6;
            ladr6->sin6_port = ret->port[l];
            llong = sizeof(*ladr6);
            break;
        default:
            printf("Normalement, ce message n'apparait pas !\n");
            exit(1);
            break;
        }
        CHK(s = socket(family, SOCK_STREAM, 0));
        r = connect(s, (struct sockaddr *) &ladr, llong);
        if (r == -1)
        {
            CHK(close(s));
            raler(1, "connect");
        }

        printf("Commande à la librairie %d\n", l);
        err = write(s, ret->datagrammes[l], ret->taille_dg[l]);
        if (err == -1)
            raler(1, "write");
        free(ret->datagrammes[l]);
        *nsock = *nsock + 1;

        FD_SET(s, fd);
        if (s > *max)
            *max = s;

        so[l] = s;    
    }
}