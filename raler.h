#ifndef RALER_
#define RALER_

#include <stdarg.h>
#include <stdnoreturn.h>
#include <fcntl.h>

/**
 * @brief Affiche un message d'erreur puis interromp l'exécution
 * @param syserr 1 si usage de perrer
 * @param fmt forme du message
 * @param ... contenu du message
 */
noreturn void raler (int syserr, const char *fmt, ...);


#endif