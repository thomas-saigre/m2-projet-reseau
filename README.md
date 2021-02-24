# Aidez les libraires !

Projet réseau M2 CSMI (d'après le sujet de Pierre David, le sujet complet est disponible [ici](https://1drv.ms/b/s!ApKvfpopznQZiLZcHMO8rJhMabWTbA))

## Description

En cette période troublée pour les commerces de proximité, vous souhaitez venir en aide aux petites librairiesindépendantes.
Pour cela, on propose de rédiger un système baptisé « Nil » pour collecter les demandes desclients et les orienter vers le ou les
libraires chez qui ils peuvent commander les livres et les chercher en mode *click and collect*. Lorsqu’un client interroge le serveur Nil,
il indique les références du ou des ouvrages souhaités. Nil interrogealors les librairies adhérentes pour déterminer lesquelles disposent
des ouvrages demandés, puis il transmetles adresses des librairies concernées aux clients. Ces derniers peuvent alors réserver les livres
directement auprès des librairies.
Le problème est que les librairies peuvent être fermées (ou en dérangement, ou déconnectées du réseau, etc.) au moment où Nil les interroge.
Il faut donc traiter les requêtes avec une certaine tolérance aux pannes. Les clients, quant à eux, souhaitent une réponse rapide de Nil
sinon ils risquent d’aller voir ailleurs.

## Programmes écrits

Les programmes suivants on été demandés :

* `librairie port livre1 livre2 ... livren`. Ce programme simule une librairie : son stock initial est indiqué par les références des
livres fournies sur la ligne de commande. Une référence peut être citée plusieurs fois si le stock contient plusieurs exemplaires du livre.
La librairie attend des requêtes du serveur Nil formulées avec UDP ainsi que des requêtes des clients formulées avec TCP pour réserver
un ou plusieurs ouvrages. Dans les deux cas, la librairie attend surle port indiqué (le même en TCP et en UDP pour simplifier).
* `nil port délai librairie port librairie port...`. Ce programme est le serveur Nil au cœur du système : chaque librairie est indiquée par son adresse
(nom ou adresse IPv4 ou IPv6) et son numéro de port UDP. Le serveur attend des requêtes des clients,formulées avec TCP sur le port indiqué en premier argument.
Dès qu’il reçoit une requête, il interroge toutes les librairies connues en utilisant UDP. Du fait des contraintes du commerce en ligne,
le délai maximum d’attente du client doit être borné parl’argument `délai` (en secondes, typiquement de l’ordre de 1 à 10 secondes),
ce qui implique que le serveur doit interroger toutes les librairies en parallèle : le serveur lance les requêtes vers les librairies les unes
après les autres, mais n’attend pas la réponse avant d’interroger la librairie suivante. Si toutes les librairies ont répondu avant l’expiration du délai,
la réponse est envoyée au client. Si une ou plusieurs librairies n’ont pas transmis de réponse à l’expiration du délai, le serveur renvoie
au client les informations en sa possession à ce moment. On notera que le serveur peut recevoir des requêtes de plusieurs clients simultanément,
ce qui impose que le serveur sache distinguer les réponses parvenant des librairies pour chaque client.
* `client serveur port livre1 livre2 ... livren`. Ce  programme  simule  un  client  :  il  interroge  le  serveur  indiqué  indifféremment par  son  nom,
son adresse IPv4 ou IPv6 ainsi que par son numéro de port TCP. Une fois les réponses obtenues, le client contacte avec TCP la ou les librairies indiquées
pour réserver les livres.


## Rapport

Le rapport de ce projet est disponible [ici](https://1drv.ms/b/s!ApKvfpopznQZiLZdapmSVFxqIUd5Mw).
