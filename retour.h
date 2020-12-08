#ifndef RETOUR_H
#define RETOUR

#include <unistd.h>
#include <inttypes.h>

#define IP_S        16
#define INIT_LIB    8

struct retour
{
    int n;              // nombre de livres
    int nlib;           // nombre de librairies
    
    int *status;        // status du livre (-1 pas encore, 0 ok, sinon 1+ind)
    char **datagrammes; // datagrammes à envoyer aux libraries

    uint8_t *type;      // IPv 4 ou 6
    uint16_t *port;     // ports de chaque librarie
    char **Ip;          // adresse IP des librairies
};

/**
 * @brief Initialise le retour du client vers les librairies
 * 
 * @param n nombre de livres
 * @param ret addresse
 */
void init_retour(const int n, struct retour *ret);

/**
 * @brief libère la mémoire
 */
void free_retour(struct retour *ret);

/**
 * @brief Renvoie l'indice correspondant à l'addresse fournie.
 *      si celle-ci n'existe pas encore, elle est ajoutée
 * 
 * @param addr addresse Ip (v4 ou v6)
 * @param port numéro de port
 * @param type type de l'ip (4 ou 6)
 * @param ret adresse du "retour"
 */
int recherche_librairie(const char* addr,const uint16_t port,const uint8_t type,
        struct retour *ret);

/**
 * @brief Ajoute un livre dans la bibliothèque et au datagramme associé
 * 
 * @param titre titre du livre
 * @param ind indice du livre dans le datagramme
 * @param ind_lib indice de la biliothèque
 * @param ret adresse du "retour"
 */
void ajouter_livre(const char *titre, const int ind, const int ind_lib,
        struct retour *ret);

/**
 * @brief Renvoie l'indice du livre dans ret, -1 si le livre n'y est pas
 * 
 * @param titre titre du livre
 * @param ret adresse du "retour"
 */
int rechercher_livre(const char *titre, struct retour *ret);

/**
 * @brief envoie les datagrammes aux librairies
 */
void envoyer_dg(struct retour *ret);

#endif