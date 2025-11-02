# awale-Game
awalé game serveur


## Archi


Chaque client rejoint avec un username

Quand on se connecte, on voit la liste des parties (possible de spectate) et des joueurs dispos
On peut challenger un joueur dispo

On peut recevoir des challenges et accepter ou non

Une fois le challenge accepté, ça lance une partie et les joueurs ne sont plus dispos

## Structure de données

## Commandes

/users -> liste des joueurs connectés
/games -> liste des parties en cours
/challenge {name_online_player}

## Questions

Comment on stocke les clients (joueurs dans le serveurs)
Comment on stocke les parties? (fifo?)
Comment on gère l'attente de la validation de la demande de partie
Comment en règle général on traite la requete? étape 1 : on checke si le joueur existe, puis on regarde son status puis ....
