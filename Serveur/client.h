#ifndef CLIENT_H
#define CLIENT_H

#include "player.h"

typedef struct {
  SOCKET sock;
  Player* player;
} Client;

#endif /* guard */
