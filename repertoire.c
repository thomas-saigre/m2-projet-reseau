#include "repertoire.h"
#include <stdlib.h>
#include "raler.h"
#include <arpa/inet.h>
#include <string.h>

#define ADD_NUMBER 256


void REP_ZERO(struct repertoire *rp)
{
    memset(rp->references, 0, CLIENT_MAX * sizeof(uint32_t));
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        rp->used[i] = 0;
        rp->datagrames[i] = NULL;
    }
}

void REP_FREE(struct repertoire *rp)
{
    for (int i=0; i<CLIENT_MAX; ++i)
        if (rp->datagrames[i] != NULL)
            free(rp->datagrames[i]);
}

void REP_ADD(const uint32_t no_commande, const int nb_livres_new,
             char *dg, int len, struct repertoire *rp)
{
    // recherche si la commande est déjà commencée
    int ind = 0;
    int is_available = 0, ind_available = -1;
    while (rp->references[ind] != no_commande && rp->used[ind] != 0
            && ind < CLIENT_MAX)
    {
        // on profite du parcours pour chercher un emplacement libre
        if (rp->used[ind] != 0 && is_available == 0)
        {
            ind_available = ind;
            is_available = 1;
        }
        ind++;
    }
    if (is_available == 0)
        raler(0, "Trop de commande simultanées, le serveur ne suit pas !");
    
    if (ind != CLIENT_MAX)  // la commande était déjà commencée
    {
        int len_old = rp->taille_dg[ind];
        int new_len = len_old + len;
        uint16_t nb_livres = ntohs(*(uint16_t *) rp->datagrames[ind]);

        rp->datagrames[ind] = realloc(rp->datagrames[ind], new_len);
        if (rp->datagrames[ind] == NULL)
            raler(0, "Erreur realloc");
        
        nb_livres += nb_livres_new;
        *(uint16_t *) rp->datagrames[ind] = htons(nb_livres);

        for (int i=0; i<len; ++i)
            rp->datagrames[ind][len_old + i] = dg[i + 2];
        
        rp->taille_dg[ind] = new_len;
    }
    else    // sinon on la crée
    {
        rp->references[ind_available] = no_commande;
        rp->taille_dg[ind_available] = len;
        rp->used[ind_available] = 1;
        rp->datagrames[ind_available] = dg;
    }
    
    

}