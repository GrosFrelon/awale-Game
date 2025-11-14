#ifndef PLAYER_H
#define PLAYER_H

#include "constants.h"
#include "types.h"

typedef struct _Game
    Game; // declaration anticipee a cause des problemes d'include circulaires

enum status_player {
  Unocupied,            // 0
  Ingame,               // 1
  Waiting_for_opponent, // 2 Le joueur attend que l'adversaire valide
  Challenge_pending,    // 3 Le joueur dit yes or no
  Waiting_for_conf_bio, // 4  Attend le Y or N pour la bio
  Writting_bio,         // 5 attend de recevoir sa string de bio
  Watching              // 6 regarde une autre game
};

typedef struct {
  int id;
  char name[NAME_SIZE];
  int elo;
  int gamesPlayed;
  int gamesWon;
  enum status_player status;
  char
      bio[BIO_SIZE]; // Obligé d'avoir un tableau et pas un pointeur sans taille
                     // fixe sinon on sait pas comment le lire coté client
  SOCKET opponent_socket; //-1 si pas de game
  Game *active_game;
} Player;

#endif /* guard */