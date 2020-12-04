#include "commande.h"
#include <stdlib.h>
#include "raler.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>




void CMD_ZERO(const int nlib, struct commande *cm)
{
    cm->nlib = nlib;
    memset(cm->references, 0, CLIENT_MAX * sizeof(uint32_t));
    memset(cm->taille_dg, 0, CLIENT_MAX * sizeof(int));
    memset(cm->used, 0, CLIENT_MAX * sizeof(int));
    memset(cm->date_send, 0, CLIENT_MAX * sizeof(time_t));
    memset(cm->recus, 0, CLIENT_MAX * sizeof(int));
    memset(cm->datagrames, 0, CLIENT_MAX * sizeof(char *));
}

void CMD_FREE(struct commande *cm)
{
    for (int i=0; i<CLIENT_MAX; ++i)
        if (cm->datagrames[i] != NULL)
            free(cm->datagrames[i]);
}

void CMD_DISP(const struct commande *cm)
{
    printf("dg : ");
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        printf("%p ", cm->datagrames[i]);
    }
    printf("\n");
    printf("used : ");
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        printf("%d ", cm->used[i]);
    }
    printf("\n");
    printf("recus : ");
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        printf("%d ", cm->recus[i]);
    }
    printf("\n");
}

void CMD_ADD(const uint32_t no_commande, const int nb_livres_new,
             const time_t date_envoi, char *dg, int len,
             struct commande *cm)
{
    printf("CMD_ADD : Réception pour commande %d\n", no_commande);
    // recherche si la commande est déjà commencée
    int ind = 0;
    int is_available = 0, ind_available = -1;
    for (ind = 0; ind < CLIENT_MAX; ++ind)
    {
        if (cm->references[ind] == no_commande && cm->used[ind] != 0)
        {
            break;
        }
        // on en profite pour chercher un emplacement libre
        if (cm->used[ind] == 0 && is_available == 0)
        {
            ind_available = ind;
            is_available = 1;
        }
    }
    printf("ind : %d\n", ind);

    if (is_available == 0 && ind == CLIENT_MAX)
        raler(0, "Trop de commande simultanées");
    
    if (ind != CLIENT_MAX)  // la commande était déjà commencée
    {
        printf("Déjà commencée %p\n", cm->datagrames[ind]);
        int len_old = cm->taille_dg[ind];
        int new_len = len_old + len;
        uint16_t nb_livres = ntohs(*(uint16_t *) cm->datagrames[ind]);

        cm->datagrames[ind] = realloc(cm->datagrames[ind], new_len);
        if (cm->datagrames[ind] == NULL)
            raler(0, "Erreur realloc");
        
        printf("%p\n", cm->datagrames[ind]);
        nb_livres += nb_livres_new;
        *(uint16_t *) cm->datagrames[ind] = htons(nb_livres);

        // for (int i=0; i<len; ++i)
        //     rp->datagrames[ind][len_old + i] = dg[i + 2];
        memcpy(&(cm->datagrames[ind][len_old]), &dg[2], len);
        free(dg);   // il faut libérer cette mémoire alloué dans ce cas
        
        cm->taille_dg[ind] = new_len;
        printf("Nb reçus, av : %d   ", cm->recus[ind]);
        cm->recus[ind] += 1;
        if (cm->recus[ind] == cm->nlib)
            // envoyer le dg
            (void) new_len;
    }
    else    // sinon on la crée
    {
        printf("Nouvelle commande\n");
        cm->references[ind_available] = no_commande;
        cm->taille_dg[ind_available] = len;
        cm->used[ind_available] = 1;
        cm->date_send[ind_available] = date_envoi;
        cm->datagrames[ind_available] = dg;
        cm->recus[ind_available] = 1;
        if (cm->recus[ind] == cm->nlib)
            // envoyer le dg
            (void) len;
    }
}

void tester_delai(struct commande *cm)
{
    time_t tps = time(NULL);
    // nb : on ne le met pas à jour à chaque fois puisque cela correspond à un 
    // nombre de secondes
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        if (cm->used[i] != 0 && cm->date_send[i] < tps)
        {
            //envoyer le dg
            printf("%d délai écoulé !\n", i);
            cm->used[i] = 0;
        }
    }
}