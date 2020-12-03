#ifndef ANNUAIRE_H
#define ANNUAIRE_H

#include <stdint.h>

struct annuaire
{
    int nlib;
    char **librairies;
    int *ports;
    int *sock;
};

/**
 * @biref initialise l'annuaire des librairies
 * 
 * @param nlib nombre de librairies
 * @param argv[] arguments (librairie1 port1 librairie2 port2 ...)
 * @param an addresse de l'annuaire
 */
void init_annuaire(int nlib, char *argv[], struct annuaire *an);

/**
 * @biref libère l'espace mémoire
 */
void free_annuaire(struct annuaire *an);

#endif