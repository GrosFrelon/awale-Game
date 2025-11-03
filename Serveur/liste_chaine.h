#ifndef LISTE_CHAINE_H
#define LISTE_CHAINE_H

#include "game.h"
#include "client.h"

typedef struct _client_node {
  Client *client;
  struct _client_node *next;
} client_node;

typedef struct _game_node {
  Game *game;
  struct _game_node *next;
} game_node;


#endif