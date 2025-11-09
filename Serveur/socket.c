#include "socket.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int send_exact(SOCKET sock, const void *buffer, size_t n) {
  size_t sent = 0;
  const char *buf = (const char *)buffer;

  while (sent < n) { // on fait une boucle pcq succès de send != tous les octets
                     // ont été envoyés. Avec la boucle, on rappelle si le send
                     // a pas terminé de tout envoyer
    int result = send(sock, buf + sent, n - sent, 0);
    if (result <= 0) { // erreur
      return -1;
    }
    sent += result;
  }
  return sent;
}

static int send_with_length(SOCKET sock, char type, const void *data,
                            size_t data_len) {
  // Message: [type:1 octet][longueur:4 octets][donnees:data_len octets]
  size_t total_len = 1 + sizeof(uint32_t) + data_len;
  char *buffer = malloc(total_len);

  if (!buffer) {
    perror("malloc");
    return -1;
  }

  buffer[0] = type;
  uint32_t net_len = htonl((uint32_t)data_len);
  memcpy(buffer + 1, &net_len, sizeof(net_len));
  memcpy(buffer + 1 + sizeof(net_len), data, data_len);

  int result = send_exact(sock, buffer, total_len);
  free(buffer);

  if (result < 0) {
    perror("send_exact");
    return -1;
  }

  return result;
}

// void send_to_client_text(Client *client, const char *message) {
//   size_t len = strlen(message);
//   char *buf = malloc(len + 2);
//   if (!buf) {
//     perror("malloc");
//     exit(errno);
//   }

//   buf[0] = '0';
//   memcpy(buf + 1, message, len);
//   if (send(client->sock, buf, len + 1, 0) < 0) {
//     perror("send()");
//     exit(errno);
//   }
//   free(buf);
// }

void send_to_client_text(Client *client, const char *message) {
  size_t len = strlen(message);
  if (send_with_length(client->sock, '0', message, len) < 0) {
    fprintf(stderr, "Erreur lors de l'envoi du texte au client\n");
  }
}

// void send_to_client_game(Client *client, jeu_t *game) {
//   size_t len = sizeof(jeu_t);
//   char *buf = malloc(len + 1);
//   if (!buf) {
//     perror("malloc");
//     exit(errno);
//   }

//   buf[0] = '1';
//   memcpy(buf + 1, game, len);

//   if (send(client->sock, buf, sizeof(jeu_t) + 1, 0) < 0) {
//     perror("send()");
//     exit(errno);
//   }
//   free(buf);
// }

void send_to_client_game(Client *client, jeu_t *game) {
  if (send_with_length(client->sock, '1', game, sizeof(jeu_t)) < 0) {
    fprintf(stderr, "Erreur lors de l'envoi du jeu au client\n");
  }
}

// void send_to_client_player(Client *client, Player *player) {
//   size_t len = sizeof(Player);
//   char *buf = malloc(len + 1);
//   if (!buf) {
//     perror("malloc");
//     exit(errno);
//   }

//   buf[0] = '2';
//   memcpy(buf + 1, player, len);

//   if (send(client->sock, buf, sizeof(Player) + 1, 0) < 0) {
//     perror("send()");
//     exit(errno);
//   }
//   free(buf);
// }

void send_to_client_player(Client *client, Player *player) {
  if (send_with_length(client->sock, '2', player, sizeof(Player)) < 0) {
    fprintf(stderr, "Erreur lors de l'envoi du joueur au client\n");
  }
}

// void send_to_client_clear(Client *client) {
//   if (send(client->sock, "3", sizeof(jeu_t) + 1, 0) < 0) {
//     perror("send()");
//     exit(errno);
//   }
// }

void send_to_client_clear(Client *client) {
  if (send_with_length(client->sock, '3', NULL, 0) < 0) {
    fprintf(stderr, "Erreur lors de l'envoi du clear au client\n");
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
