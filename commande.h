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
    int desc[CLIENT_MAX];               // descripteur du client
    time_t date_send[CLIENT_MAX];       // date à laquelle le délai est dépassé
    int recus[CLIENT_MAX];              // nombre de réponses reçues
    char *datagrammes[CLIENT_MAX];      // datagramme à envoyer au client
};

/**
 * @brief Initialise le répertoire
 */
void init_commande(const int nlib, struct commande *cm);

/**
 * @brief utilisée à des fins de déboguage
 */
void afficher_commande(const struct commande *cm);

/**
 * @brief initialise la commande
 * 
 * @param no_commande numéro de la commande
 * @param desc descripteur de sortie pour le client
 * @param date_envoi date où le délai est dépassé
 * @param cm addresse de la base de données de commandes
 * @return 0 si tout s'est bien passé, -1 si CLIENT_MAX est atteint
 */
int nouvelle_commande(const uint32_t no_commande, const int desc,
            const time_t date_envoi, struct commande *cm);

/**
 * @biref complète la commande, celle ci doit être initiée
 * 
 * @param no_commande numéro de la commande
 * @param nb_livres nombre de livre à ajouter à la commande
 * @param datagramme données à envoyer au client
 * @param len taille du datagramme
 * @param cm adresse des commanes
 */
void ajouter_commande(const uint32_t no_commande, const int nb_livres,
            char *datagramme, const int len, struct commande *cm);

/**
 * @brief libère l'espace mémoire alloué pour le répertoire
 */
void free_commande(struct commande *cm);


/**
 * @brief teste si le délai des commandes est atteint
 */
void tester_delai(struct commande *cm);

/**
 * @brief Envoie le résultat de la commande au client
 * 
 * @param ind indice de la commande dans la base de données
 * @param cm addresse de la base de données
 */
void envoyer_reponse(const int ind, struct commande *cm);


#endif