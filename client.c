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
    for (int i=ind+1; i<max_livre; ++i)
    {
        if (memcmp(titre, &dg[2 + i*REPONSE_S], TITRE_S) == 0)
        {
            ind_liv = 2 + i*REPONSE_S;
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
int trouver_indice_du_livre(const char *titre, char *livre[], const int *recu, const int n)
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

    struct retour ret;
    init_retour(max_livre, &ret);

    int ind = 2, ind_lib, ind_livre, present;
    int sd;
    char titre[TITRE_S + 1];
    titre[TITRE_S] = '\0';
    uint8_t type;
    char IP[IP_S];
    uint16_t port;

    int *indices = calloc(n, sizeof(int));  // indice du livre dans dg_rep
    int nb_reponse = 0;                     // compteur du nombre de réponses reçues
    int *recu = calloc(n, sizeof(int));     // booléens pour savie si le livre est traité définitivement


    // Tant qu'on a pas eu de réponse (postive ou négative définitive) pour chaque livre
    while (nb_reponse < n)
    {
        // On recherche la présence ou non des livres dans le dg_nil
        for (uint16_t l = 0; l < n; ++l)
        {
            // on vérifie qu'on n'a pas déjà eu une réponse définitive
            if (recu[l] == 0)
            {
                // on recherche si le livre est dans la réponse
                present = rechercher_dans_dg(livre[l], buf_rep, indices[l], max_livre);
                if (present == -1)
                {
                    recu[l] = 1;
                    nb_reponse++;
                    printf("Référence %s non disponible\n", livre[l]);
                }
                else
                {
                    ind = 2 + present * REPONSE_S;
                    indices[l] = present;
                    type = *(uint8_t *) &buf_rep[ind + TITRE_S];
                    memcpy(IP, &buf_rep[ind + TITRE_S + 1], IP_S);
                    port = ntohs(*(uint16_t *) &buf_rep[ind + TITRE_S + 1 + IP_S]);

                    ind_lib = recherche_librairie(IP, port, type, &ret);
                    ajouter_livre(livre[l], present, ind_lib, &ret);
                }
            }
        }

        printf("On en est ici\n");
        disp(&ret);

        // envoi des datagrammes aux librairies
        fd_set readfds;
        int sockarray[MAXSOCK];
        int max, nsock = 0;
        FD_ZERO(&readfds);
        envoyer_dg(&readfds, &max, sockarray, &nsock, &ret);

        if (select(max+1, &readfds, NULL, NULL, NULL) == -1)
        {
            syslog(LOG_ERR, "%s: %m", "select");
            exit(1);
        }
        
        char rep_lib[MAXLEN];
        for (int i=0; i<nsock; ++i)
        {
            struct sockaddr_storage sonadr;
            socklen_t salong;
            void *nadr;
            uint16_t *no_port;
            int af;
            char padr[INET6_ADDRSTRLEN];

            if (FD_ISSET(sockarray[i], &readfds))
            {
                sd = accept(sockarray[i], (struct sockaddr *) &sonadr, &salong);
                r = recvfrom(sd, rep_lib, MAXLEN, 0, (struct sockaddr *) &sonadr, &salong);
                af = ((struct sockaddr *) &sonadr)->sa_family;
                // uint8_t type;
                switch (af)
                {
                case AF_INET :
                    nadr = &((struct sockaddr_in *) &sonadr)->sin_addr;
                    // type = IPv4;
                    no_port = &((struct sockaddr_in *) &sonadr)->sin_port;
                    break;
                case AF_INET6 :
                    nadr = &((struct sockaddr_in6 *) &sonadr)->sin6_addr;
                    // type = IPv6;
                    no_port = &((struct sockaddr_in6 *) &sonadr)->sin6_port;
                    break;
                }
                char *adr_;
                adr_ = nadr;


                uint16_t nb_ret = ntohs(*(uint16_t *) rep_lib);
                // uint16_t nb_ret = recherche_librairie(nadr, *no_port, type,
                                    // &ret);
                uint8_t rep;
                ind = 2;
                for (int l=0; l<nb_ret; ++l)
                {
                    rep = *(uint8_t *) &buf_rep[ind + TITRE_S + l*(TITRE_S+1)];
                    if (rep == 0)   // le livre n'a pas été commandé
                    {
                        memcpy(titre, &rep_lib[ind], TITRE_S);
                        int ind_liv = rechercher_livre(titre, &ret);
                        int ind_dg = rechercher_dans_dg(titre, buf_rep,
                                        ind_liv, max_livre);

                        if (ind_dg == -1)
                        {
                            printf("%s non disponible\n", titre);
                            nb_reponse++;
                            ind_livre = trouver_indice_du_livre(titre, livre, recu, n);
                            if (ind_livre == -1)
                            {
                                printf("En toute logique, ce message n'apparaîtra jamais\n");
                            }
                            recu[ind_livre] = 1;
                            nb_reponse++;
                        }
                    }
                    else        // le livre a été commandé
                    {
                        inet_ntop(af, nadr, padr, sizeof padr);
                        printf("%s commandé sur %s/%hn\n",titre, adr_, no_port);
                        nb_reponse++;
                            ind_livre = trouver_indice_du_livre(titre, livre, recu, n);
                            if (ind_livre == -1)
                            {
                                printf("En toute logique, ce message n'apparaîtra jamais\n");
                            }
                            recu[ind_livre] = 1;
                            nb_reponse++;
                    }
                    ind += TITRE_S + 1;
                }
            }
        }
    }

    free(indices);
    free(recu);
    free_retour(&ret);
}



int main(int argc, char *argv[])
{
    if (argc < 3) usage(argv[0]);

    int nb_livres = argc - 3;

    gerer_requete(argv[1], argv[2], nb_livres, &argv[3]);

    return 0;
}