#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"

static void init(void) {
#ifdef WIN32
  WSADATA wsa;
  int err = WSAStartup(MAKEWORD(2, 2), &wsa);
  if (err < 0) {
    puts("WSAStartup failed !");
    exit(EXIT_FAILURE);
  }
#endif
}

static void end(void) {
#ifdef WIN32
  WSACleanup();
#endif
}

static void app(const char *address, const char *name) {
  SOCKET sock = init_connection(address);
  char buffer[BUF_SIZE];

  fd_set rdfs;

  /* send our name */
  write_server(sock, name);

  while (1) {
    FD_ZERO(&rdfs);

    /* add STDIN_FILENO */
    FD_SET(STDIN_FILENO, &rdfs);

    /* add the socket */
    FD_SET(sock, &rdfs);

    if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1) {
      perror("select()");
      exit(errno);
    }

    /* something from standard input : i.e keyboard */
    if (FD_ISSET(STDIN_FILENO, &rdfs)) {
      fgets(buffer, BUF_SIZE - 1, stdin);
      {
        char *p = NULL;
        p = strstr(buffer, "\n");
        if (p != NULL) {
          *p = 0;
        } else {
          /* fclean */
          buffer[BUF_SIZE - 1] = 0;
        }
      }
      write_server(sock, buffer);
    } else if (FD_ISSET(sock, &rdfs)) {
      // int n = read_server(sock, buffer);
      // /* server down */
      // if (n == 0) {
      //   printf("Server disconnected !\n");
      //   break;
      // }
      // afficher_buffer(buffer, n);
      process_server_message(sock);
    }
  }

  end_connection(sock);
}

static int init_connection(const char *address) {
  SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  SOCKADDR_IN sin = {0};
  struct hostent *hostinfo;

  if (sock == INVALID_SOCKET) {
    perror("socket()");
    exit(errno);
  }

  hostinfo = gethostbyname(address);
  if (hostinfo == NULL) {
    fprintf(stderr, "Unknown host %s.\n", address);
    exit(EXIT_FAILURE);
  }

  sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
  sin.sin_port = htons(PORT);
  sin.sin_family = AF_INET;

  if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR) {
    perror("connect()");
    exit(errno);
  }

  return sock;
}

static void end_connection(int sock) { closesocket(sock); }

// static int read_server(SOCKET sock, char *buffer) {
//   int n = 0;

//   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
//     perror("recv()");
//     exit(errno);
//   }

//   buffer[n] = 0;

//   return n;
// }

static int recv_exact(SOCKET sock, void *buffer, size_t n) {
  size_t received = 0;
  char *buf = (char *)buffer;

  while (received < n) {
    int r = recv(sock, buf + received, n - received, 0);
    if (r <= 0) {
      return r; // Erreur ou déconnexion
    }
    received += r;
  }
  return received;
}

static int receive_message(SOCKET sock, char *buffer, size_t buffer_size,
                           char *msg_type) {
  // Recevoir le type (1 octet)
  char type;
  if (recv_exact(sock, &type, 1) != 1) {
    return -1;
  }
  *msg_type = type;

  // Recevoir la longueur (4 octets)
  uint32_t net_len;
  if (recv_exact(sock, &net_len, sizeof(net_len)) != sizeof(net_len)) {
    return -1;
  }

  uint32_t data_len = ntohl(net_len);

  // pour pas se retrouver avec la moitie des donnees dans le buffer
  if (data_len > buffer_size) {
    fprintf(stderr, "Erreur : message trop long: %u octets (buffer: %zu)\n",
            data_len, buffer_size);
    return -1;
  }

  if (data_len == 0) {
    return 0;
  }

  if (recv_exact(sock, buffer, data_len) != (int)data_len) {
    return -1;
  }

  return data_len;
}

static void write_server(SOCKET sock, const char *buffer) {
  if (send(sock, buffer, strlen(buffer), 0) < 0) {
    perror("send()");
    exit(errno);
  }
}

static void afficher_jeu(jeu_t jeu) {
  for (int i = 0; i < 6; i++) {
    printf("%d ", jeu.plateau[i]);
  }
  printf("  <- ligne J1\n");
  for (int i = 11; i > 5; i--) {
    printf("%d ", jeu.plateau[i]);
  }
  printf("  <- ligne J2\n");
  printf("Score du J1 : %d \n", jeu.j1Score);
  printf("Score du J2 : %d \n", jeu.j2Score);
}

static void afficher_player(Player player) {
  printf("Name : %s\n", player.name);
  printf("Bio : %s\n", player.bio);
  printf("Nombre de victoires : %d\n", player.gamesWon);
  printf("Nombre de games jouées : %d\n", player.gamePlayed);
}

static void process_server_message(SOCKET sock) {
  char buffer[BUF_SIZE];
  char msg_type;

  int len = receive_message(sock, buffer, sizeof(buffer), &msg_type);

  if (len < 0) {
    fprintf(stderr, "Erreur de réception\n");
    return;
  }

  switch (msg_type) {
  case '0':             // Message texte
    buffer[len] = '\0'; // Terminer la chaîne
    printf("%s", buffer);
    fflush(stdout);
    break;

  case '1': // État du jeu
    if (len == sizeof(jeu_t)) {
      jeu_t jeu;
      memcpy(&jeu, buffer, sizeof(jeu_t));
      // afficher_jeu(jeu);
      afficher_jeu_ascii_art(jeu);
    } else {
      fprintf(stderr, "Paquet jeu_t incomplet (%d/%zu)\n", len, sizeof(jeu_t));
      fprintf(stderr, "Buffer (ascii): ");
      for (int i = 0; i < len; i++) {
        fprintf(stderr, "%c", (unsigned char)buffer[i]);
      }
      fprintf(stderr, "\n");
      ;
    }
    break;

  case '2': // Informations joueur
    if (len == sizeof(Player)) {
      Player player;
      memcpy(&player, buffer, sizeof(Player));
      afficher_player(player);
    } else {
      fprintf(stderr, "Taille de joueur invalide\n");
    }
    break;

  case '3': // Clear screen
    printf("\033[2J\033[H");
    fflush(stdout);
    break;

  default:
    fprintf(stderr, "Type de message inconnu: '%c'\n", msg_type);
    break;
  }
}

// static void afficher_buffer(char *buffer, int n) {
//   if (n > 0) {
//     if (buffer[0] == '0') {
//       if (n > 1) {
//         puts(buffer + 1);
//       }
//     } else if (buffer[0] == '1') {
//       // Données d’un jeu_t en binaire après l’indicateur
//       if (n - 1 >= (int)sizeof(jeu_t)) {
//         jeu_t jeu;
//         memcpy(&jeu, buffer + 1, sizeof(jeu_t));
//         afficher_jeu(jeu);
//       } else {
//         fprintf(stderr, "Paquet jeu_t incomplet (%d/%zu)\n", n - 1,
//                 sizeof(jeu_t));
//       }
//     } else if (buffer[0] == '2') {
//       // On peut pas tester la taille minimal parcque taille dépend de la
//       taille
//       // de la bio et du name (ou alors faut prendre la taille sans bio et
//       sans
//       // name)
//       Player player;
//       memcpy(&player, buffer + 1, sizeof(Player));
//       afficher_player(player);
//     } else if (buffer[0] == '3') {
//       printf("\033[2J\033[H");
//       fflush(stdout);
//     } else {
//       puts(buffer);
//     }
//   }
// }

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage : %s [address] [pseudo]\n", argv[0]);
    return EXIT_FAILURE;
  }

  init();

  app(argv[1], argv[2]);

  end();

  return EXIT_SUCCESS;
}

static void afficher_jeu_ascii_art(jeu_t jeu) {

  printf("\t");
  for (int i = 0; i < (37 - strlen(jeu.j1Name)) / 2; i++) {
    printf(" ");
  }
  printf("%s", jeu.j1Name);
  for (int i = 0; i < 37 - strlen(jeu.j1Name) - ((37 - strlen(jeu.j1Name)) / 2);
       i++) {
    printf(" ");
  }
  if (jeu.rotation == -1) {
    printf("<┐");
  } else {
    printf(" ┐");
  }
  printf("\n\t┌─────┬─────┬─────┬─────┬─────┬─────┐");
  if (jeu.rotation == 1) {
    printf(" v");
  }
  printf("\n\t│");
  for (int i = 0; i < 6; i++) {
    if (jeu.plateau[i] < 10) {
      printf("  %d  |", jeu.plateau[i]);
    } else {
      printf("  %d |", jeu.plateau[i]);
    }
  }
  printf("\n\t├───────────────────────────────────┤\n\t│");
  for (int i = 11; i > 5; i--) {
    if (jeu.plateau[i] < 10) {
      printf("  %d  |", jeu.plateau[i]);
    } else {
      printf("  %d |", jeu.plateau[i]);
    };
  }
  printf("\n\t└─────┴─────┴─────┴─────┴─────┴─────┘\n");
  printf("\t");
  for (int i = 0; i < (37 - strlen(jeu.j2Name)) / 2; i++) {
    printf(" ");
  }
  printf("%s\n", jeu.j2Name);
  for (int i = 0; i < (37 - strlen(jeu.j2Name) - (37 - strlen(jeu.j2Name))) / 2;
       i++) {
    printf(" ");
  }
  printf("Score de %s : %d \n", jeu.j1Name, jeu.j1Score);
  printf("Score de %s : %d \n", jeu.j2Name, jeu.j2Score);
}