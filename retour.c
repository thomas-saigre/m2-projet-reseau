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

#define CHK(v) do{if((v)==-1)raler(1,#v);} while(0)

void init_retour(const int n, struct retour *ret)
{
    ret->n = n;
    ret->current_ind = 0;
    ret->nlib = INIT_LIB;

    // ret->ind = malloc(n * sizeof(int));
    // if (ret->ind == NULL) raler(0, "Malloc");
    // memset(ret->ind, -1, n * sizeof(int));

    // ret->titre = malloc(n * sizeof(char *));
    // if (ret->titre == NULL) raler(0, "Malloc");
    // for (int i=0; i<n; ++i)
    //     ret->titre[i] = "";

    ret->datagrammes = malloc(INIT_LIB * sizeof(char *));
    if (ret->datagrammes == NULL) raler(0, "Malloc");
    memset(ret->datagrammes, 0, INIT_LIB * sizeof(char *));

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

    ret->sock = malloc(INIT_LIB * sizeof(int));
    if (ret->sock == NULL) raler(1, "malloc");
    memset(ret->sock, -1, INIT_LIB * sizeof(int));
}

void disp(const struct retour *ret)
{
    printf("\nLibrairies (%d)\n", ret->nlib);
    for (int i = 0; i < ret->nlib; ++i)
    {
        printf("%d : %p %d %d - IP ", i, ret->datagrammes[i], ret->type[i], ret->port[i]);
        for (int k=0; k<16; ++k)
            printf("%d ", ret->Ip[i][k]);
        printf("\n");
    }
}

void free_retour(struct retour *ret)
{
    // free(ret->ind);
    // free(ret->titre);
    // on libère la mémoire des datagrammes au moment de l'envoi
    free(ret->datagrammes);
    free(ret->type);
    free(ret->port);
    free(ret->Ip);
    free(ret->sock);
}

int recherche_librairie(const char* addr,const uint16_t port,const uint8_t type,
        struct retour *ret)
{
    int ind = 0;
    int ind_libre, is_libre = 0;

    // on recherche si on a déjà commencé un dg pour la librairie
    for (; ind<ret->nlib; ++ind)
    {
        // printf("%s\n", ret->Ip[ind]);//, ret->port[ind]);


        if ((strncmp(addr, ret->Ip[ind], IP_S) == 0) &&
             port == ret->port[ind])
        {
            // printf("on passe par ici\n");
            break;
        }
        if ((ret->port[ind] == 0) && (is_libre == 0))
        {
            // printf("et par là\n");
            is_libre = 1;
            ind_libre = ind;
        }
    }

    // printf("is_libre %d\n", is_libre);
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

            ret->datagrammes = realloc(ret->datagrammes, ret->nlib * sizeof(char *));
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

            ret->sock = realloc(ret->sock, ret->nlib * sizeof(int));
            if (ret->sock == NULL) raler(0, "realloc");
            ret->sock[ret->nlib-1] = -1;

            return ret->nlib - 1;
        }
        
    }
    
}


void ajouter_livre(char *titre, const int ind_lib, struct retour *ret)
{
    // ret->ind[ret->current_ind] = ind;
    // ret->titre[ret->current_ind] = titre;

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
        int old_ind = ret->taille_dg[ind_lib];
        ret->datagrammes[ind_lib] = realloc(ret->datagrammes[ind_lib],
                ret->taille_dg[ind_lib] + TITRE_S);
        if (ret->datagrammes[ind_lib] == NULL)
            raler(0, "realloc");
        memset(&ret->datagrammes[ind_lib][ret->taille_dg[ind_lib]], 0, TITRE_S);

        uint16_t old_nb_liv = ntohs(*(uint16_t *) ret->datagrammes[ind_lib]);
        *(uint16_t *) ret->datagrammes[ind_lib] = htons(old_nb_liv + 1);

        // int ind_tmp = (ret->taille_dg[ind_lib] - 2) / TITRE_S;
        strncpy(&ret->datagrammes[ind_lib][old_ind], titre, TITRE_S);

        ret->taille_dg[ind_lib] += TITRE_S;
    }  
}


void envoyer_dg(fd_set *fd, int *max, struct retour *ret)
{
    // on ouvre une socket par librairie
    for (int l=0; l<ret->nlib; ++l)
    {
        if (ret->datagrammes[l] != NULL)
        {
            // printf("librairie %d\n", l);
            int af, r;
            char padr[INET6_ADDRSTRLEN], port[6];
            snprintf(port, 6, "%d", ret->port[l]);
            switch (ret->type[l])
            {
            case IPv4:
                af = AF_INET;
                break;
            case IPv6:
                af = AF_INET6;
                break;
            default:
                printf("Normalement, ce message n'apparait pas ! type=%d\n", ret->type[l]);
                break;
            }
            inet_ntop(af, ret->Ip[l], padr, sizeof(padr));

            struct addrinfo hints, *res, *res0;
            memset(&hints, 0, sizeof hints);
            hints.ai_family = PF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            // printf("getaddrinfo vers %s/%s\n", padr, port);

            r = getaddrinfo(padr, port, &hints, &res0);
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
                        raler(1, "connect");
                        close(s);
                        s = -1;
                    }
                    else break;
                }
                
            }
            if (s == -1) raler(0, "Erreur : %s", cause);
            freeaddrinfo(res0);

#ifdef DISPLAY
            printf("Commande à la librairie %d : ", l);
            for (int i=0; i<ret->taille_dg[l]; ++i)
                printf("%d ", ret->datagrammes[l][i]);
            printf("\n");
#endif
            r = write(s, ret->datagrammes[l], ret->taille_dg[l]);
            if (r == -1)
                raler(1, "write");
            free(ret->datagrammes[l]);
            ret->datagrammes[l] = NULL;
            

            FD_SET(s, fd);
            if (s > *max)
                *max = s;

            ret->sock[l] = s;
            printf("%d à l'envoi sur %d\n", l, ret->sock[l]);
        }
    }
}