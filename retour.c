#include "retour.h"
#include "raler.h"
#include <stdlib.h>
#include <string.h>

void init_retour(const int n, struct retour *ret)
{
    ret->n = n;
    ret->nlib = INIT_LIB;
    ret->status = malloc(n * sizeof(int));
    if (ret->status == NULL) raler(0, "Malloc");
    memset(ret->status, -1, n * sizeof(int));

    ret->datagrammes = malloc(n * sizeof(char *));
    if (ret->datagrammes == NULL) raler(0, "Malloc");
    memset(ret->datagrammes, 0, n * sizeof(char *));
    
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

void free_retour(struct retour *ret)
{
    free(ret->status);
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
            return ind_libre;
        }
        else                // sinon on rajoute une case
        {
            ret->nlib += 1;
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

void ajouter_livre(const char *titre, const int ind, const int ind_lib,
         struct retour *ret)
{
    // TODO
}

int rechercher_livre(const char *titre, struct retour *ret)
{
    // TODO
}

void envoyer_dg(struct retour *ret)
{
    // TODO
}