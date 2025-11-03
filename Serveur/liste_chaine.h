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

void create_client(Client *client, client_node *list_clients);
void create_game(Game *game, game_node *list_games);
void delete_game(Game *game, game_node *list_games);
void delete_client(Client *client, client_node *list_clients);

#endif