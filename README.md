# Awale Game - Jeu en réseau

Un jeu d'Awale multijoueur en ligne développé en C avec architecture client-serveur.

## 📋 Description

Ce projet implémente le jeu traditionnel africain Awale en version multijoueur réseau. Les joueurs peuvent se défier, discuter et regarder les parties en cours.

## 🎮 Fonctionnalités

### Côté serveur
- Gestion de plusieurs clients simultanés (jusqu'à 100)
- Système de défis entre joueurs
- Sauvegarde persistante des profils joueurs
- Mode spectateur pour observer les parties en cours
- Chat entre joueurs non occupés et en cours de partie
- Gestion des statistiques (victoires, parties jouées)

### Côté client
- Interface pour l'affichage du plateau
- Commandes interactives
- Profil joueur avec biographie personnalisable
- Visualisation des parties en direct

## 🛠️ Compilation

Le projet utilise un Makefile pour la compilation :

```bash
make all
```

Cela génère deux exécutables :
- `server.app` : le serveur de jeu
- `client.app` : le client de jeu

On peut aussi générer les exécutables indépendamment avec : 
- `make server.app` pour le côté serveur
- `make client.app` pour le côté client

### Nettoyage

```bash
make clean
```

## 🚀 Utilisation

### Lancer le serveur

```bash
./server.app
```

Le serveur écoute sur le port **1977** par défaut.

Pour sauvegarder et arrêter le serveur, appuyez sur une touche dans le terminal du serveur.

### Lancer le client

```bash
./client.app <adresse_serveur> <pseudo>
```

Exemple :
```bash
./client.app 127.0.0.1 Raph
```

## 📖 Commandes disponibles

### En dehors d'une partie

| Commande | Description |
|----------|-------------|
| `/help` | Affiche l'aide des commandes |
| `/me` | Affiche vos informations de profil |
| `/users` | Liste les joueurs connectés (disponibles et en jeu) |
| `/player <nom>` | Affiche les informations d'un joueur |
| `/challenge <nom>` | Défie un joueur à une partie |
| `/spectate <nom>` | Regarde la partie d'un joueur |
| `/bio` | Modifie votre biographie |
| `/chat <nom> <message>` | Envoie un message privé à un joueur |

### Pendant une partie

| Commande | Description |
|----------|-------------|
| `1-6` | Joue la case correspondante |
| `!<message>` | Envoie un message à l'adversaire |
| `q` | Abandonne la partie |
| `?` | Affiche l'aide en jeu |

### En mode spectateur

Appuyez sur n'importe quelle touche pour arrêter de regarder la partie.

## 🎯 Règles du jeu Awale

- Le plateau contient 12 cases (6 par joueur) avec 4 graines par case au départ
- À tour de rôle, les joueurs prennent toutes les graines d'une de leurs cases et les sèment dans le sens de rotation
- Si la dernière graine tombe dans une case adverse contenant 1 ou 2 graines (donc 2 ou 3 après le dépôt), le joueur capture ces graines
- Le joueur peut continuer à capturer les graines des cases adverses précédentes si elles contiennent aussi 2 ou 3 graines
- **Règle de famine** : un joueur ne peut pas jouer un coup qui laisse l'adversaire sans graines
- Le premier joueur à capturer 24 graines (ou plus) gagne

## 📁 Structure du projet

```
.
├── Client/
│   ├── client.c         # Code principal du client
│   ├── client.h
│   ├── jeu.h           # Structure du jeu
│   └── player.h        # Structure du joueur
├── Serveur/
│   ├── awale.c         # Logique du jeu Awale
│   ├── awale.h
│   ├── client.h        # Structure client
│   ├── constants.h     # Constantes (port, tailles buffers)
│   ├── game.h          # Structure de partie
│   ├── liste_chaine.c  # Gestion de la liste de parties
│   ├── liste_chaine.h
│   ├── player.h        # Structure joueur serveur
│   ├── server.c        # Logique serveur principale
│   ├── server.h
│   ├── socket.c        # Gestion des sockets et protocole
│   ├── socket.h
│   └── types.h         # Types réseau
├── Makefile
├── save.txt            # Fichier de sauvegarde des joueurs
└── README.md
```

## 🔧 Protocole réseau

Le projet utilise un protocole personnalisé avec préfixes de type :

- **Type '0'** : Message texte
- **Type '1'** : État du jeu (structure `jeu_t`)
- **Type '2'** : Informations joueur (structure `Player`)
- **Type '3'** : Commande de nettoyage d'écran

Format : `[type:1 octet][longueur:4 octets][données:N octets]`

## 💾 Sauvegarde

Les profils joueurs sont automatiquement sauvegardés dans [`save.txt`](save.txt) lors de l'arrêt du serveur. Le fichier contient :
- ID unique
- Nom
- Biographie
- Nombre de parties jouées
- Nombre de victoires
- ELO (pour une implémentation future)

## 👥 Auteurs

- Raphaël LETOURNEUR
- Alois PINTO DE SILVA -- WINNEFELD

## 📝 Configuration

Les paramètres principaux sont définis dans [`Serveur/constants.h`](Serveur/constants.h) :

```c
#define PORT 1977              // Port du serveur
#define MAX_CLIENTS 100        // Nombre max de clients
#define BUF_SIZE 1024          // Taille du buffer
#define NAME_SIZE 50           // Taille max du nom
#define BIO_SIZE 900           // Taille max de la bio
#define SAVE_FILE "save.txt"   // Fichier de sauvegarde
```

## 🐛 Limitations connues

- Le serveur doit être arrêté proprement (appui sur une touche) pour sauvegarder les données
- Pas de reconnexion automatique en cas de déconnexion
- Pas de système de classement ELO fonctionnel (structure présente pour implémentation future)

## 📜 Notes

Ce projet a été développé dans le cadre du cours de Programmation Réseaux de 4e année à l'INSA Lyon.
