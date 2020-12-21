#include "stock.h"
#include "raler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TITRE_S 10




void free_stock(struct stock *st)
{
    free(st->livres);
    free(st->disp);
}

void init_stock(int n, char *argv[], struct stock *st)
{
    st->n = n;
    st->livres = (char *) calloc(n * TITRE_S, sizeof(char));
    st->disp = (int *) malloc(n * sizeof(int));

    if (st->livres == NULL || st->disp == NULL)
    {
        raler(0, "erreur malloc");
    }

    for (int i=0; i<n; ++i)
    {
        int n_ecrits = snprintf(&st->livres[i*TITRE_S], TITRE_S, "%s", argv[i]);
        st->disp[i] = 1;
        if (n_ecrits > TITRE_S)
        {
            free_stock(st);
            raler(0, "Erreur : Titre %s trop long", argv[i]);
        }
    }
}

int reserver_livre(const char *titre, struct stock *st)
{
    int ind = -1;
    for (int i=0; i<st->n; ++i)
    {
        if ((strcmp(&st->livres[i*TITRE_S], titre) == 0) && st->disp[i])
        {
            ind = i;
            st->disp[i] = 0;
            break;
        }
    }
    return ind;
}

int est_disponible(const char *titre, const struct stock *st)
{
    int ind = -1;
    for (int i=0; i<st->n; ++i)
    if ((strcmp(&st->livres[i*TITRE_S], titre) == 0) && st->disp[i])
        {
            ind = i;
            break;
        }
    return ind;
}

void afficher_stock(const struct stock *st)
{
    printf("Stock disponible : ");
    for (int i = 0; i < st->n; ++i)
        if (st->disp[i]) printf("%s ", &st->livres[i*TITRE_S]);
    printf("\n\n");
}