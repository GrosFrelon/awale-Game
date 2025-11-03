#ifndef PLAYER_H
#define PLAYER_H

#include "server.h"

enum status_player {
  Unocupied,  //0
  Ingame,  //1
  Waiting   //2
};

typedef struct {
  char id[6];
  char name[BUF_SIZE];
  int elo;
  int gamePlayed;
  int gamesWon;
  enum status_player status;
  char* bio;
} Player;

#endif /* guard */