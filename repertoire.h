#ifndef REPERTOIRE_H
#define REPERTOIRE_H

#include <stdint.h>
#define MAXLEN      1024
#define CLIENT_MAX  128

struct repertoire
{
    uint32_t references[CLIENT_MAX];    // no de référence de la commande
    int taille_dg[CLIENT_MAX];          // taille du dg à envoyer
    int used[CLIENT_MAX];               // 0 si la case est disponible
    char *datagrames[CLIENT_MAX];       // datagramme à envoyer au client
};

/**
 * @brief Initialise le répertoire
 */
void REP_ZERO(struct repertoire *rp);

void REP_DISP(const struct repertoire *re);

/**
 * @biref ajoute une commande au répertoire,
 * si la commande existait déjà, elle est complétée
 * 
 * @param no_commande numéro de la commande
 * @param nb_livres nombre de livre à ajouter à la commande
 * @param datagramme données à envoyer au client
 * @param len taille du datagramme
 * @param rp adresse du répertoire
 */
void REP_ADD(const uint32_t no_commande, const int nb_livres,
             char *datagramme, const int len, struct repertoire *rp);

/**
 * @brief libère l'espace mémoire alloué pour le répertoire
 */
void REP_FREE(struct repertoire *rp);


#endif