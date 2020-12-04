#ifndef COMMANDE_H
#define COMMANDE_H

#include <stdint.h>
#include <time.h>
#define MAXLEN      1024
#define CLIENT_MAX  32 


struct commande
{
    int nlib;                           // nombre total de librairies
    uint32_t references[CLIENT_MAX];    // no de référence de la commande
    int taille_dg[CLIENT_MAX];          // taille du dg à envoyer
    int used[CLIENT_MAX];               // 0 si la case est disponible
    time_t date_send[CLIENT_MAX];       // date à laquelle le délai est dépassé
    int recus[CLIENT_MAX];              // nombre de réponses reçues
    char *datagrames[CLIENT_MAX];       // datagramme à envoyer au client
};

/**
 * @brief Initialise le répertoire
 */
void CMD_ZERO(const int nlib, struct commande *cm);

void CMD_DISP(const struct commande *cm);

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
void CMD_ADD(const uint32_t no_commande, const int nb_livres,
             const time_t date_envoi, char *datagramme, const int len,
             struct commande *cm);

/**
 * @brief libère l'espace mémoire alloué pour le répertoire
 */
void CMD_FREE(struct commande *cm);


/**
 * @brief teste si le délai des commandes est atteint
 */
void tester_delai(struct commande *cm);


#endif