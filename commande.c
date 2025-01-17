#include "commande.h"
#include <stdlib.h>
#include "raler.h"
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define CHK(v) do{if((v)==-1)raler(1,#v);} while(0)


void init_commande(const int nlib, struct commande *cm)
{
    cm->nlib = nlib;
    memset(cm->references, 0, CLIENT_MAX * sizeof(uint32_t));
    memset(cm->taille_dg, 0, CLIENT_MAX * sizeof(int));
    memset(cm->used, 0, CLIENT_MAX * sizeof(int));
    memset(cm->desc, -1, CLIENT_MAX * sizeof(int));
    memset(cm->date_send, 0, CLIENT_MAX * sizeof(time_t));
    memset(cm->recus, 0, CLIENT_MAX * sizeof(int));
    memset(cm->datagrammes, 0, CLIENT_MAX * sizeof(char *));
    // on n'alloue pas la mémoire des dg tout de suite car cela ferait beaucoup
}

void free_commande(struct commande *cm)
{
    for (int i=0; i<CLIENT_MAX; ++i)
        if (cm->datagrammes[i] != NULL)
            free(cm->datagrammes[i]);
}

void afficher_commande(const struct commande *cm)
{
    printf("dg : ");
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        printf("%p ", cm->datagrammes[i]);
    }
    printf("\n");
    printf("used : ");
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        printf("%d ", cm->used[i]);
    }
    printf("\n");
    printf("desc : ");
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        printf("%d ", cm->desc[i]);
    }
    printf("\n");
    printf("recus : ");
    for (int i=0; i<CLIENT_MAX; ++i)
    {
        printf("%d ", cm->recus[i]);
    }
    printf("\n");
}

void reponse_erreur(const int desc)
{
    printf("Trop de commandes simultanées, envoi dg -1\n");
    char dg[2];
    memset(dg, -1, 2*sizeof(char));
    int r = write(desc, dg, 2);
    if (r == -1) raler(1, "write");

    CHK(close(desc));    
}

int nouvelle_commande(const uint32_t no_commande, const int desc,
        const time_t date_envoi, struct commande *cm)
{
    int ind = 0;
    while (ind < CLIENT_MAX && cm->used[ind] != 0)
        ind++;
    if (ind == CLIENT_MAX)
    {
        reponse_erreur(desc);
        return -1;
    }
    cm->used[ind] = 1;
    cm->references[ind] = no_commande;
    cm->desc[ind] = desc;
    cm->date_send[ind] = date_envoi;
    cm->taille_dg[ind] = 0;
    cm->recus[ind] = 0;
    // on ne change pas le reste pour éviter de faire une copie
    return 0;
}

// avant d'utiliser cette fonction, utiliser nouvelle_commande pour initialiser
void ajouter_commande(const uint32_t no_commande, const int nb_livres_new,
             char *dg, int len, struct commande *cm)
{
    int ind = 0;
    while (ind < CLIENT_MAX && cm->references[ind] != no_commande)
    {
        ind++;
    }

    if (ind == CLIENT_MAX)
        raler(0, "Erreur inexplicable dans ajouter_commande");

    if (cm->taille_dg[ind] != 0) //si on a déjà commencé à remplir le datagramme
    {
        int len_old = cm->taille_dg[ind];
        int new_len = len_old + len - 2;
        if (new_len > MAXLEN)
            raler(0, "Trop de réponse, datagramme trop petit :/");

        uint16_t nb_livres = ntohs(*(uint16_t *) cm->datagrammes[ind]);

        cm->datagrammes[ind] = realloc(cm->datagrammes[ind], new_len);
        if (cm->datagrammes[ind] == NULL)
            raler(0, "Erreur realloc");
        
        nb_livres += nb_livres_new;
        *(uint16_t *) cm->datagrammes[ind] = htons(nb_livres);

        memcpy(&(cm->datagrammes[ind][len_old]), &dg[2], len - 2);
        free(dg);   // il faut libérer cette mémoire alloué dans ce cas
        
        cm->taille_dg[ind] = new_len;
        cm->recus[ind] += 1;


        if (cm->recus[ind] == cm->nlib)
            envoyer_reponse(ind, cm);
    }
    else
    {
        cm->references[ind] = no_commande;
        cm->taille_dg[ind] = len;
        cm->datagrammes[ind] = dg;
        cm->recus[ind] = 1;
        if (cm->recus[ind] == cm->nlib)
        {
            envoyer_reponse(ind, cm);
        }
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
            printf("Délai pour %d écoulé\n", cm->references[i]);
            if (cm->taille_dg[i] == 0)  // si on n'a reçu aucune réponse,
            {                           //  le dg n'existe pas encore
                cm->datagrammes[i] = calloc(2, sizeof(char));
                cm->taille_dg[i] = 2;
            }
            envoyer_reponse(i, cm);
        }
    }
}


void envoyer_reponse(const int ind, struct commande *cm)
{
    int r;
    printf("Réponse envoyée au client\n\n");
    r = write(cm->desc[ind], cm->datagrammes[ind], cm->taille_dg[ind]);
    if (r == -1) raler(1, "write");

    CHK(close(cm->desc[ind]));
    cm->used[ind] = 0;
    cm->desc[ind] = -1;

    // on libère la mémoire alloué au datagramme envoyé
    free(cm->datagrammes[ind]);
    cm->datagrammes[ind] = NULL;
}