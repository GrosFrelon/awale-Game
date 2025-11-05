#include "socket.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void send_to_client_text(Client *client, const char *message) {
  size_t len = strlen(message);
  char *buf = malloc(len + 1);
  if (!buf) { perror("malloc"); exit(errno); }

  buf[0] = '0';
  memcpy(buf + 1, message, len);
  if (send(client->sock, message, strlen(message), 0) < 0) {
    perror("send()");
    exit(errno);
  }
}

void send_to_client_game(Client *client, jeu_t *game) {
  size_t len = sizeof(jeu_t);
  char *buf = malloc(len + 1);
  if (!buf) { perror("malloc"); exit(errno); }

  buf[0] = '1';
  memcpy(buf + 1, game, len);

  if (send(client->sock, game, sizeof(jeu_t), 0) < 0) {
    perror("send()");
    exit(errno);
  }
}

// Fonctions du prof

int init_connection(void) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  SOCKADDR_IN sin = {0};

  if (sock == INVALID_SOCKET) {
    perror("socket()");
    exit(errno);
  }

  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(PORT);
  sin.sin_family = AF_INET;

  if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR) {
    perror("bind()");
    exit(errno);
  }

  if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR) {
    perror("listen()");
    exit(errno);
  }

  return sock;
}

int read_client(SOCKET sock, char *buffer) {
  int n = 0;

  if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
    perror("recv()");
    /* if recv error we disonnect the client */
    n = 0;
  }

  buffer[n] = 0;

  return n;
}

void clear_clients(Client *clients, int actual) {
  int i = 0;
  for (i = 0; i < actual; i++) {
    closesocket(clients[i].sock);
  }
}

void end_connection(int sock) { closesocket(sock); }
