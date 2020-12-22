#include "retour.h"
#include "raler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>



#define IPv4    4
#define IPv6    6
#define IPv4LEN 32
#define IPv6LEN 128
#define MAXLEN  1024

#define CHK(v) do{if((v)==-1)raler(1,#v);} while(0)

void init_retour(const int n, struct retour *ret)
{
    ret->n = n;
    ret->nlib = INIT_LIB;

    ret->datagrammes = malloc(INIT_LIB * sizeof(char *));
    if (ret->datagrammes == NULL) raler(0, "Malloc");
    for (int i=0; i<INIT_LIB; ++i)
    {
        ret->datagrammes[i] = calloc(MAXLEN, sizeof(char));
        if (ret->datagrammes[i] == NULL) raler(0, "calloc");
    }

    ret->taille_dg = malloc(INIT_LIB * sizeof(int));
    if (ret->taille_dg == NULL) raler(0, "malloc");
    memset(ret->taille_dg, 0, INIT_LIB*sizeof(int));
    
    ret->type = malloc(INIT_LIB * sizeof(uint8_t));
    if (ret->type == NULL) raler(0, "Malloc");
    // type à -1 signifie qu'il n'y a rien à utiliser
    memset(ret->type, -1, INIT_LIB * sizeof(uint8_t));
    
    ret->port = calloc(INIT_LIB, sizeof(uint16_t));
    if (ret->port == NULL) raler(0, "Malloc");
    
    ret->Ip = malloc(INIT_LIB * sizeof(char *));
    if (ret->Ip == NULL) raler(0, "Malloc");
    for (int i=0; i<INIT_LIB; ++i)
    {
        ret->Ip[i] = calloc(IP_S, sizeof(char));
        if (ret->Ip[i] == NULL) raler(0, "Malloc");
    }
}


void free_retour(struct retour *ret)
{
    for (int i = 0; i < ret->nlib; ++i)
        free(ret->datagrammes[i]);
    free(ret->datagrammes);
    free(ret->taille_dg);
    free(ret->type);
    free(ret->port);
    for (int i = 0; i < ret->nlib; ++i)
        free(ret->Ip[i]);
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


        if ((strncmp(addr, ret->Ip[ind], IP_S) == 0) &&
             port == ret->port[ind])
        {
            break;
        }
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

            ret->datagrammes = realloc(ret->datagrammes,
                    ret->nlib * sizeof(char *));
            if (ret->datagrammes == NULL) raler(0, "realloc");
            ret->datagrammes[ret->nlib-1] = calloc(MAXLEN, sizeof(char));
            if (ret->datagrammes[ret->nlib-1] == NULL) raler(0, "calloc");

            ret->taille_dg = realloc(ret->taille_dg, ret->nlib*sizeof(int));
            if (ret->taille_dg == NULL) raler(0, "realloc");
            ret->taille_dg[ret->nlib-1] = 0;

            ret->type = realloc(ret->type, ret->nlib * sizeof(uint8_t));
            if (ret->type == NULL) raler(0, "realloc");
            ret->type[ret->nlib-1] = type;

            ret->port = realloc(ret->port, ret->nlib * sizeof(uint16_t));
            if (ret->port == NULL) raler(0, "realloc");
            ret->port[ret->nlib-1] = port;

            ret->Ip = realloc(ret->Ip, ret->nlib * sizeof(char *));
            if (ret->Ip == NULL) raler(0, "realloc");
            ret->Ip[ret->nlib-1] = malloc(IP_S * sizeof(char));
            memcpy(ret->Ip[ret->nlib-1], addr, IP_S);

            return ret->nlib - 1;
        }   // if is_libre
    }   // if ind < nlib
}   // fonction rechercher librairie


void ajouter_livre(char *titre, const int ind_lib, struct retour *ret)
{
    if (ret->taille_dg[ind_lib] == 0)   // on n'a pas encore commencé à écrire
    {                                   //  le dg
        *(uint16_t *) ret->datagrammes[ind_lib] = htons(1);
        strncpy(&ret->datagrammes[ind_lib][2], titre, TITRE_S);

        ret->taille_dg[ind_lib] = 2 + TITRE_S;
    }
    else
    {
        int old_ind = ret->taille_dg[ind_lib];
        int new_len = ret->taille_dg[ind_lib] + TITRE_S;
        if (new_len > MAXLEN)
            raler(0, "Trop de livre à commander à la librairie");

        uint16_t old_nb_liv = ntohs(*(uint16_t *) ret->datagrammes[ind_lib]);
        *(uint16_t *) ret->datagrammes[ind_lib] = htons(old_nb_liv + 1);

        strncpy(&ret->datagrammes[ind_lib][old_ind], titre, TITRE_S);

        ret->taille_dg[ind_lib] += new_len;
    }  
}
