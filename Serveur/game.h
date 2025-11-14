#ifndef GAME_H
#define GAME_H

#include "awale.h"
#include "client.h"
#include "player.h"

typedef struct _Game {
  int game_id;
  Player *player1;
  Player *player2;
  int playing;
  int winner;
  jeu_t jeu;
  Client **watchers;
  int watchers_list_size;
  int nb_watchers;
} Game;

#endif /* guard */