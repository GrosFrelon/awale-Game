#ifndef PLAYER_H
#define PLAYER_H

#include "../Serveur/constants.h"

enum status_player {
  Unocupied,            // 0
  Ingame,               // 1
  Waiting_for_opponent, // 2  Le joueur attend que l'adversaire valide
  Challenge_pending,    // 3  Le joueur dit yes or no
  Waiting_for_conf_bio, // 4  Attend le Y or N pour la bio
  Writting_bio          // 5 attend de recevoir sa string de bio
};

typedef struct _Player {
  int id;
  char name[NAME_SIZE];
  int elo;
  int gamesPlayed;
  int gamesWon;
  enum status_player status;
  char bio[BIO_SIZE];
  int opponent_socket; //-1 si pas de game
  char _padding_active_game[sizeof(void *)];
} Player;

#endif