#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"

enum etatJoueur {
  Unocupied,  //0
  Ingame,  //1
  Waiting   //2
};

typedef struct {
  SOCKET sock;
  char name[BUF_SIZE];
  enum etatJoueur status;
} Client;

#endif /* guard */
