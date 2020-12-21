#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include "raler.h"
#include "retour.h"

#define TITRE_S    10
#define REPONSE_S  29
#define MAXLEN     1024
#define MAXSOCK    64
#define IPv4       4
#define IPv6       6

#define CHK(v) do{if((v)==-1)raler(1,#v);} while(0)

void usage (char *argv0)
{
    fprintf(stderr, "usage: %s serveur port livre1 libre2 ... livren\n", argv0);
    exit (1) ;
}

/**
 * @brief Recherche dans le datagramme de réponse de nil si le livre est présent
 *        Il s'agit de l'indice au sein du dg (0 <= ind < maxlivre)
 * 
 * @param titre titre du livre
 * @param dg datagramme réponse de nil
 * @param ind indice à partir duquel rechercher
 * @param max_livre nombre total de livres dans le dg
 * @result indice (dans le dg) si le livre y est, -1 sinon
 */
int rechercher_dans_dg(const char *titre, const char *dg,
        const int ind, const int max_livre)
{
    int ind_liv = -1;
    // printf("  ind %d\n", ind+1);
    for (int i=ind+1; i<max_livre; ++i)
    {
        // printf("\tComparaison %s avec %s :", titre, &dg[2+i*REPONSE_S]);
        int cmp = strncmp(titre, &dg[2 + i*REPONSE_S], TITRE_S);
        // printf("%d\n", cmp);
        if (cmp == 0)
        {
            ind_liv = i;
            break;
        }
    }
    return ind_liv;
}


/**
 * @brief Trouve l'indice d'un livre non traité dans la liste de départ
 * 
 * @param titre titre du livre
 * @param livre[] références des livres
 * @param recu status des livres
 * @param n nombre total de livres
 */
int trouver_indice_du_livre(const char *titre, char *livre[], const int *recu,
        const int n)
{
    int ind = -1;
    for (int i = 0; i < n; ++i)
    {
        if ((strncmp(titre, livre[i], TITRE_S) == 0) && (recu[i] == 0))
        {
            ind = i;
            break;
        }
    }
    return ind;
}


/**
 * @brief gère la commande du client
 * @param serveur addresse sur serveur (IPv4 ou IPv6)
 * @param port_nil port TCP du serveur
 * @param n nombre de livre à commander
 * @param livre références des ouvrages à commander
 */
void gerer_requete(const char *serveur, const char *port_nil,
    const uint16_t n, char *livre[])
{
    int err;

    const int taille_dg = 2 + n*TITRE_S;

    // on peut commander au plus 102 livres (donc c'est large)
    if (taille_dg > MAXLEN)
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
    struct addrinfo hints, *res, *res0;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

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
    char buf_rep[MAXLEN];
    printf("\nAttente réponse\n");
    err = read(s, buf_rep, MAXLEN);
    if (err == -1) raler(1, "read");

    printf("Réponse reçue, nb octets lus %d\n", err);
    printf("\t%s\n", buf_rep);

    // fermeture de la connexion
    close(s);
    free(datagramme);

// Client <-> librairies
    uint16_t max_livre = ntohs(*(u_int16_t *) buf_rep);

#ifdef DISPLAY
    // on affiche le dg recu
    printf("DG deçu de nil : ");
    for (int i=0; i<err; ++i)
        printf("%d ", buf_rep[i]);
    printf("\n\n");
#endif

    struct retour ret;
    init_retour(max_livre, &ret);

    int ind = 2, ind_lib, present;
    char titre[TITRE_S + 1];
    int ind_livre;
    memset(titre, 0, TITRE_S + 1);
    uint8_t type;
    char IP[IP_S];
    uint16_t port;

    int *indices = malloc(n * sizeof(int)); // indice du livre dans dg_rep
    if (indices == NULL)
        raler(1, "malloc");
    memset(indices, -1, n*sizeof(int));
    int nb_reponse = 0;                  // compteur du nombre de réponses
    int *recu = calloc(n, sizeof(int));  // livre traité définitivement ou non
    if (recu == NULL)
        raler(1, "Calloc");


    // Tant qu'on a pas eu de réponse (postive ou négative définitive)
    //  pour chaque livre
    while (nb_reponse < n)
    {
        // On recherche la présence ou non des livres dans le dg_nil
        for (uint16_t l = 0; l < n; ++l)
        {
            // on vérifie qu'on n'a pas déjà eu une réponse définitive
            if (recu[l] == 0)
            {
                // on recherche si le livre est dans la réponse
                present = rechercher_dans_dg(livre[l], buf_rep,
                            indices[l], max_livre);
                if (present == -1)
                {
                    recu[l] = 1;
                    nb_reponse++;
                    printf("%s non disponible\n", livre[l]);
                }
                else
                {
                    ind = 2 + present * REPONSE_S;
                    indices[l] = present;
                    type = *(uint8_t *) &buf_rep[ind + TITRE_S];
                    memcpy(IP, &buf_rep[ind + TITRE_S + 1], IP_S);
                    port = ntohs(*(uint16_t *)
                            &buf_rep[ind + TITRE_S + 1 + IP_S]);

                    ind_lib = recherche_librairie(IP, port, type, &ret);
                    ajouter_livre(livre[l], ind_lib, &ret);
                }
            }
        }

        if (nb_reponse == n)    // pas besoin de continuer
            break;


        char rep_lib[MAXLEN];
        memset(rep_lib, 0, MAXLEN);

        // envoi des datagrammes aux librairies et reception de la réponse

        // on ouvre une socket par librairie
        for (int l = 0; l < ret.nlib; ++l)
        {
            if (ret.taille_dg[l] > 0) // si il y a qqch à envoyer
            {
                int af, r;
                char padr[INET6_ADDRSTRLEN], port[6];
                snprintf(port, 6, "%d", ret.port[l]);
                switch (ret.type[l])
                {
                case IPv4:
                    af = AF_INET;
                    break;
                case IPv6:
                    af = AF_INET6;
                    break;
                default:
                    printf("Normalement, ce message n'apparait pas ! type=%d\n",
                           ret.type[l]);
                    break;
                }
                inet_ntop(af, ret.Ip[l], padr, sizeof(padr));

                struct addrinfo hints, *res, *res0;
                memset(&hints, 0, sizeof hints);
                hints.ai_family = PF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;

                // printf("getaddrinfo vers %s/%s\n", padr, port);

                r = getaddrinfo(padr, port, &hints, &res0);
                if (r != 0) raler(0, "getaddrinfo: %s\n", gai_strerror(r));

                int s = -1;
                char *cause;
                for (res = res0; res != NULL; res = res->ai_next)
                {
                    s = socket(res->ai_family, res->ai_socktype,
                               res->ai_protocol);
                    if (s == -1) cause = "socket";
                    else
                    {
                        r = connect(s, res->ai_addr, res->ai_addrlen);
                        if (r == -1)
                        {
                            cause = "connect";
                            raler(1, "connect");
                            close(s);
                            s = -1;
                        }
                        else break;
                    }
                    
                }
                if (s == -1) raler(0, "Erreur : %s", cause);
                freeaddrinfo(res0);

#ifdef DISPLAY
                printf("Commande à la librairie %d : ", l);
                for (int i=0; i<ret->taille_dg[l]; ++i)
                    printf("%d ", ret->datagrammes[l][i]);
                printf("\n");
#endif
                r = write(s, ret.datagrammes[l], ret.taille_dg[l]);
                if (r == -1)
                    raler(1, "write");
                free(ret.datagrammes[l]);
                ret.datagrammes[l] = NULL;


                // On attend la réponse dans la foulée
                CHK(r = read(s, rep_lib, MAXLEN));
                CHK(close(s));

#ifdef DISPLAY
                printf("DG reçu de la lib : ");
                for (int i=0; i<r; ++i)
                    printf("%d ", rep_lib[i]);
                printf("\n");
#endif



                uint16_t nb_ret = ntohs(*(uint16_t *) rep_lib);
                uint8_t rep;
                ind = 2;

                switch (ret.type[l])
                {
                case IPv4:
                    af = AF_INET;
                    break;
                case IPv6:
                    af = AF_INET6;
                    break;
                default:
                    printf("Normalement, ce message n'apparait pas ! type=%d\n",
                           ret.type[l]);
                    break;
                }

                for (int liv = 0; liv < nb_ret; ++liv)
                {
                    rep = *(uint8_t *) &rep_lib[ind + TITRE_S];
                    memcpy(titre, &rep_lib[ind], TITRE_S);

                    switch (rep)
                    {
                    case 1:         // le livre a été commandé
                        inet_ntop(af, ret.Ip[l], padr, sizeof(padr));
                        printf("%s commandé sur %s/%d\n", titre, padr,
                               ret.port[l]);
                        nb_reponse++;
                        ind_livre = trouver_indice_du_livre(titre,livre,recu,n);
                        if (ind_livre == -1)
                        {
                            printf("En toute logique, ce message n'apparaîtra\
                                   jamais\n");
                        }
                        recu[ind_livre] = 1;
                        break;

                    case 0:
                        // on ne fait rien ici, ce sera de retour dans la boucle
                        // principale qu'on va voir si on peut recommander le
                        // livre ailleurs
                        break;
                    
                    default:
                        printf("La magie noire vous aura ammené ici %d\n", rep);
                        break;
                    }

                    ind += TITRE_S + 1;
                }   // for liv = 0 to nb_ret
            }   // if dg != NULL
        }   // for sur les librairies
    }   // while nb_reponse < n

    free(indices);
    free(recu);
    free_retour(&ret);
}   // fonction gerer_requete



int main(int argc, char *argv[])
{
    if (argc < 3) usage(argv[0]);

    int nb_livres = argc - 3;

    gerer_requete(argv[1], argv[2], nb_livres, &argv[3]);

    printf("\nFin\n");

    return 0;
}