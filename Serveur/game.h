#ifndef GAME_H
#define GAME_H

#include "../awale.h"
#include "player.h"

typedef struct {
  int game_id;
  Player* player1;
  Player* player2;
  int playing;
  int winner;
  jeu_t jeu;
} Game;

#endif /* guard */