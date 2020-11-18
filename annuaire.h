#ifndef ANNUAIRE_H
#define ANNUAIRE_H

struct annuaire
{
    int nlib;
    char **librairies;
    int *ports;
};

void init_annuaire(int nlib, char *argv[], struct annuaire *an);

void free_annuaire(struct annuaire *an);

#endif