#ifndef STOCK_H
#define STOCK_H

struct stock
{
    int n;              // nombre de livres
    char *livres;       // titres des livres
    int *disp;          // disponiblilités
};

/**
 * @brief initialise la structure stock
 * 
 * @param n nombre de livres dans le stock
 * @param argv[] tableau de taille n contenant les références
 * @result struct stock * (donné en paramètre)
 */
void init_stock(int n, char *argv[], struct stock *st);

/**
 * @brief libère la mémoire allouée pour le stock
 * 
 * @param *st pointeur vers la structure
 */
void free_stock(struct stock *st);

/**
 * @brief effectue une réservation : enlève un livre du sotck
 * 
 * @param titre référence du livre
 * @result indice du livre dans le sotck, -1 si le livre n'est pas présent
 */
int reserver_livre(const char *titre, struct stock *st);


/**
 * @brief renvoie la référence du livre dans le sotck (-1 sinon)
 *
 * @param titre référence du livre
 * @result indice du livre
 */
int est_disponible(const char *titre, const struct stock *st);

/**
 * @brief affiche le stock encore disponible
 */
void afficher_stock(const struct stock *st);

#endif