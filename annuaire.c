#include "annuaire.h"
#include <stdlib.h>
#include "raler.h"

void init_annuaire(int nlib, char *argv[], struct annuaire *an)
{
    an->nlib = nlib;
    an->librairies = malloc(nlib * sizeof(char *));
    if (an->librairies == NULL)
        raler(0, "Erreur malloc");
    an->ports = malloc(nlib * sizeof(int));
    if (an->ports == NULL)
        raler(0, "Erreur malloc");
    an->sock = malloc(nlib * sizeof(int));
    if (an->sock == NULL)
        raler(0, "Erreur malloc");

    int ind = 0;
    for (int i=0; i<nlib; ++i)
    {
        an->librairies[i] = argv[ind];
        an->ports[i] = atoi(argv[ind+1]);
        ind += 2;
    }
}

void free_annuaire(struct annuaire *an)
{
    free(an->librairies);
    free(an->ports);
    free(an->sock);
}
