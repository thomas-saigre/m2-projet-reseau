#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "raler.h"
#include "retour.h"

#define TITRE_S   10
#define MAX_LEN   1024

#define CHK(v) do{if((v)==-1)raler(1,#v);} while(0)

void usage (char *argv0)
{
    fprintf(stderr, "usage: %s serveur port livre1 libre2 ... livren\n", argv0);
    exit (1) ;
}

/**
 * @brief Recherche dans le datagramme de réponse de nil si le livre est présent
 * 
 * @param titre titre du livre
 * @param dg datagramme réponse de nil
 * @param ind indice à partir duquel rechercher
 * @result indice si le livre y est  -1 sinon
 */
int rechercher_dans_dg(const char *titre, const char *dg,
        const int ind, const int max_livre)   // TODO
{

    return -1;
}

/**
 * @brief gère la commande du client
 * @param serveur addresse sur serveur (IPv4 ou IPv6)
 * @param port_nil port TCP du serveur
 * @param n nombre de livre à commander
 * @param livre référence de l'ouvrage
 */
void gerer_requete(const char *serveur, const char *port_nil,
    const uint16_t n, char *livre[])
{
    int err;

    const int taille_dg = 2 + n*TITRE_S;

    // on peut commander au plus 102 livres (donc c'est large)
    if (taille_dg > MAX_LEN)
        raler(0, "Trop de commandes");

    char *datagramme = calloc(taille_dg, sizeof(char));

    *(uint16_t *) datagramme = htons(n);
    int i_dg = 2;
    for (int i=0; i<n; ++i)
    {
        int n_ecrits = snprintf(&datagramme[i_dg], 10, "%s", livre[i]);
        if (n_ecrits > 10) raler(0, "Titre %s trop long", livre[i]);
        i_dg += TITRE_S;
    }
// Client <-> Nil
    // ouverture de la connection TCP vers nil
    struct addrinfo hints, *res, *res0 ;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC ;
    hints.ai_socktype = SOCK_STREAM ;

    int r = getaddrinfo(serveur, port_nil, &hints, &res0);
    if (r != 0) raler(0, "getaddrinfo: %s\n", gai_strerror(r));

    int s = -1;
    char *cause;
    for (res = res0; res != NULL; res = res->ai_next)
    {
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s == -1) cause = "socket";
        else
        {
            r = connect(s, res->ai_addr, res->ai_addrlen);
            if (r == -1)
            {
                cause = "connect";
                close(s);
                s = -1;
            }
            else break;
        }
    }
    if (s == -1) raler(0, "Erreur : %s", cause);
    freeaddrinfo(res0);

    // envoi de la requête
    printf("Envoi requête à %s/%s\n", serveur, port_nil);
    err = write(s, datagramme, taille_dg);
    if (err == -1) raler(1, "Échec envoi requête");

    // attente de la réponse
    char buf_rep[MAX_LEN];
    printf("\nAttente réponse\n");
    err = read(s, buf_rep, MAX_LEN);
    if (err == -1) raler(1, "read");

    printf("Réponse reçue, nb octets lus %d\n", err);
    printf("\t%s\n", buf_rep);

    // fermeture de la connexion
    close(s);
    free(datagramme);

// Client <-> librairies
    uint16_t max_livre = ntohs(*(u_int16_t *) buf_rep);

    struct retour ret;
    init_retour(max_livre, &ret);

    int a_envoyer = 0;
    int ind = 2, ind_lib, ind_livre;
    char titre[TITRE_S + 1];
    titre[TITRE_S] = '\0';
    uint8_t type;
    char IP[IP_S];
    uint16_t port;

    // on commence par rechercher la première occurence de chaque livre
    for (uint16_t l=0; l<max_livre; ++l)
    {
        memcpy(titre, &buf_rep[ind], TITRE_S);

        ind_livre = rechercher_livre(titre, &ret);
        if (ind_livre == -1)    // si le livre n'est pas déjà dedans
        {
            type = *(uint8_t *) &buf_rep[ind + TITRE_S];
            memcpy(IP, &buf_rep[ind + TITRE_S + 1], IP_S);
            port = *(uint16_t *) &buf_rep[ind + TITRE_S + 1 + IP_S];
            
            ind_lib = recherche_librairie(IP, port, type, &ret);
            ajouter_livre(titre, l, ind_lib, &ret);
            a_envoyer = 1;
        }
    }

    while (a_envoyer == 1)
    {
        envoyer_dg(&ret);
        a_envoyer = 0;
        
        // select()         // TODO
        for (;;)    // boucle avec fd_set TODO
        {
            char rep_lib[MAX_LEN];
            CHK(r = read(0, rep_lib, MAX_LEN)); // TODO read dans la socket

            uint16_t nb_ret = 0;    // TODO nb de retour du db de la lib
            uint8_t rep;
            for (int l=0; l<nb_ret; ++l)
            {
                rep = 0;    // TODO status du livre dans la réponse
                if (rep == 0)
                {
                    int ind_liv = rechercher_livre("", &ret);   // TODO titre
                    int ind_dg = rechercher_dans_dg("", buf_rep, ind_liv, max_livre);

                    if (ind_dg == -1)
                    {
                        printf("!! non disponible\n");  // TODO titre
                    }
                    else
                    {
                        ajouter_livre("", ind_dg, 0, &ret);    // TODO titre + ind_lib
                        a_envoyer = 1;
                    }
                    
                }
                else
                {
                    printf("!! commandé sur ::1 9001\n");   // TODO tire + ip+port
                }
                
            }
        }
    }

    // size_t size_dg = 2 + nb_livres * TITRE_S;

    // char **datagrammes = malloc(n * sizeof(char *));
    // uint8_t *val = calloc(n, sizeof(uint8_t));

    // for (size_t l = 0; l < nb_livres; ++l)
    // {

    // }


    // free(dg_send);
    // free(val);
}



int main(int argc, char *argv[])
{
    if (argc < 3) usage(argv[0]);

    int nb_livres = argc - 3;

    gerer_requete(argv[1], argv[2], nb_livres, &argv[3]);

    return 0;
}