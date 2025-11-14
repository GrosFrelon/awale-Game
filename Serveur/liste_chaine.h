#ifndef LISTE_CHAINE_H
#define LISTE_CHAINE_H

#include "game.h"

typedef struct _game_node {
  Game *game;
  struct _game_node *next;
} game_node;

void create_game(Game *game, game_node **list_games);
void delete_game(Game *game, game_node **list_games);
Game *find_game_with_player(Player *player, game_node **list_games);

#endif