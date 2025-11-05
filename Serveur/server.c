#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "liste_chaine.h"
#include "server.h"
#include "socket.h"

#include "client.h"
#include "game.h"
#include "liste_chaine.h"

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

static unsigned int g_next_id =
    1; // TODO, changer pour que ca redémarre pas à 1 à chaque redemarage

static void app(void) {
  SOCKET sock = init_connection();
  char buffer[BUF_SIZE];
  /* the index for the array */
  int actual = 0;
  int max = sock;
  int nombre_player = 0;
  int taille_liste_player = 10;
  int id = 0;
  /* an array for all clients and players */
  Client clients[MAX_CLIENTS];
  Player **players = malloc(taille_liste_player * sizeof(Player *));
  game_node *list_games = NULL;

  fd_set rdfs; // structure de données pour sauvegarder ce qu'on surveille

  while (1) {
    int i = 0;
    FD_ZERO(&rdfs);

    /* add STDIN_FILENO */
    FD_SET(STDIN_FILENO,
           &rdfs); // on surveille l'entrée standard (clavier). Si l'utilisateur
                   // tape quelque chose, FD_ISSET(STDIN_FILENO, &rdfs) sera
                   // vrai après select.

    /* add the connection socket */
    FD_SET(sock, &rdfs);

    /* add socket of each client */
    for (i = 0; i < actual; i++) {
      FD_SET(clients[i].sock, &rdfs);
    }

    if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1) {
      perror("select()");
      exit(errno);
    }

    /* something from standard input : i.e keyboard */
    if (FD_ISSET(STDIN_FILENO, &rdfs)) {
      /* stop process when type on keyboard */
      break;
    } else if (FD_ISSET(sock, &rdfs)) {
      /* new client */
      SOCKADDR_IN csin = {0};
      socklen_t sinsize = sizeof csin;
      int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
      if (csock == SOCKET_ERROR) {
        perror("accept()");
        continue;
      }

      /* after connecting the client sends its name */
      if (read_client(csock, buffer) == -1) {
        /* disconnected */
        continue;
      }

      /* what is the new maximum fd ? */
      max = csock > max ? csock : max;

      FD_SET(csock, &rdfs);

      int fist_co = 0;
      Player *p = find_player_by_name(buffer, players, nombre_player);
      if (p == 0) {
        p = malloc(sizeof(Player));
        p->elo = 0;
        p->gamePlayed = 0;
        p->gamesWon = 0;
        p->status = Unocupied;
        p->id = id++;
        p->opponent_socket = -1;
        strncpy(p->name, buffer, NAME_SIZE);
        add_player(&players, p, &nombre_player, &taille_liste_player);
        fist_co = 1;
      }

      Client c = {.sock = csock, .player = p};
      clients[actual] = c;

      actual++;
      send_welcome_message(&c, fist_co);
    } else {
      int i = 0;
      for (i = 0; i < actual; i++) {
        /* a client is talking */
        if (FD_ISSET(clients[i].sock, &rdfs)) {
          Client client = clients[i];
          int c = read_client(clients[i].sock, buffer);
          /* client disconnected */
          if (c == 0) {
            closesocket(clients[i].sock);
            remove_client(clients, i, &actual);
          } else {
            Client *sender = &clients[i];
            if(sender->player->status == Waiting_for_opponent || sender->player->status == Challenge_pending){
              Client* opponent = find_client_by_socket(sender->player->opponent_socket, clients, actual);
              handle_challenge_response(sender, opponent ,buffer, clients, list_games);
            }
            else if (sender->player->status == Unocupied) {
              analyse_command(&clients[i], buffer, clients, actual, list_games);
            }
            else if (sender->player->status == Ingame){
              handle_game_move(sender, buffer);
            }
          }
          break;
        }
      }
    }
  }

  clear_clients(clients, actual);
  end_connection(sock);
}

static void remove_client(Client *clients, int to_remove, int *actual) {
  /* we remove the client in the array */
  memmove(clients + to_remove, clients + to_remove + 1,
          (*actual - to_remove - 1) * sizeof(Client));
  /* number client - 1 */
  (*actual)--;
}

static void add_player(Player ***players, Player *player, int *nombre_player,
                       int *taille_liste) {
  if (*nombre_player >= *taille_liste) {
    int new_cap = *taille_liste + 10;
    Player **new_arr = realloc(*players, new_cap * sizeof(Player *));
    *players = new_arr;
    *taille_liste = new_cap;
  }
  (*players)[*nombre_player] = player;
  (*nombre_player)++;
}

static void send_unoccupied_clients(Client *clients, Client *sender,
                                    int actual) {
  int i = 0;
  char message[BUF_SIZE];
  message[0] = 0;

  strncpy(message, "List of online players unocupied:\n",
          sizeof(message) - strlen(message) - 1);

  for (i = 0; i < actual; i++) {
    if (clients[i].player->status == Unocupied &&
        clients[i].sock != sender->sock) {
      strncat(message, "\t- ", sizeof(message) - strlen(message) - 1);
      strncat(message, clients[i].player->name,
              sizeof(message) - strlen(message) - 1);
      strncat(message, "\n", sizeof(message) - strlen(message) - 1);
    }
  }
  send_to_client_text(sender, message);
}

static void analyse_command(Client *sender, const char *buffer, Client *clients,
                            int actual, game_node *list_games) {
  if (buffer[0] == '/') {
    if (strncmp(buffer, "/users", 6) == 0) {
      send_unoccupied_clients(clients, sender, actual);
    } else if (strncmp(buffer, "/challenge", 10) == 0) {
      char receiver_name[BUF_SIZE];
      if (sscanf(buffer + 10, " %s", receiver_name) != 1) {
        send_to_client_text(sender, "Usage: /challenge <name>\n");
        return;
      }
      send_request_challenge(sender, receiver_name, clients, actual,
                             list_games);
    } else {
      send_to_client_text(sender, "bad command");
    }
  }
}

static void send_request_challenge(Client *sender, char *receiver,
                                   Client *clients, int actual,
                                   game_node *list_games) {
  Client *pClient = is_client_unocupied(clients, receiver, actual);
  char message[BUF_SIZE];
  message[0] = 0;
  if (pClient == 0) {
    sprintf(message, "Receiver not found : %s", receiver);
    send_to_client_text(sender, message);
  } else if (pClient->sock == sender->sock) {
    sprintf(message, "You can't play against yourself");
    send_to_client_text(sender, message);
  } else { // Comment on différencie les clients? par le sock ou le nom. En vrai
    // pareil parcqu'on retrouve la personne avec son nom donc nom unique
    sprintf(message, "Challenge request send to : %s", pClient->player->name);
    send_to_client_text(sender, message);

    message[0] = 0;
    sprintf(message, "%s challenges you \n Press Y to accept, N to reject",
            sender->player->name);
    send_to_client_text(pClient, message);

    pClient->player->status = Challenge_pending;
    sender->player->status = Waiting_for_opponent;

    sender->player->opponent_socket = pClient->sock;
    pClient->player->opponent_socket = sender->sock;
  }
}

static void handle_challenge_response(Client* sender, Client* opponent, char* buffer, Client* clients, game_node* list_games){

  char message[BUF_SIZE];
  message[0] = 0;

  if ((strcmp(buffer, "Y") == 0 || strcmp(buffer, "y") == 0) && sender->player->status == Challenge_pending) {
      start_game(opponent, sender, list_games);
  } else if (strcmp(buffer, "N") == 0 || strcmp(buffer, "n") == 0) {
    message[0] = 0;
    sprintf(message, "%s  refused the challenge\n", sender->player->name);
    send_to_client_text(opponent, message);
    opponent->player->status = Unocupied;
    sender->player->status = Unocupied;
  }
}

static void start_game(Client *client1, Client *client2,
                       game_node *list_games) {
  char message[BUF_SIZE];
  char buffer[BUF_SIZE];
  message[0] = 0;
  sprintf(message, "New game started between %s and %s", client1->player->name,
          client2->player->name);
  send_to_client_text(client1, message);
  send_to_client_text(client2, message);
  client1->player->status = Ingame;
  client2->player->status = Ingame;

  jeu_t *jeu = initGame(-1);

  Game *game = malloc(sizeof(Game));
  game->game_id = 0;
  game->jeu = *jeu;
  game->player1 = client1->player;
  game->player2 = client2->player;
  game->playing = 1;
  game->winner = 0;

  create_game(game, list_games);

  send_to_client_game(client1, jeu);
  send_to_client_game(client2, jeu);

  srand(time(NULL));
  int player = (rand() % 2);
  int positionDemande = 0;

  if (player + 1 == 1) {
      send_to_client_text(client1, "C'est votre tour, donnez le numéro de la "
                                  "case que vous voulez jouer -> ");
      send_to_client_text(client2, "Ce n'est pas votre tour...\n");
  } else {
    send_to_client_text(client1, "Ce n'est pas votre tour...\n");
    send_to_client_text(client2, "C'est votre tour, donnez le numéro de la "
                                "case que vous voulez jouer -> ");
  }


  // while (jeu->j1Score < 24 || jeu->j2Score < 24) {
  //   printf("coup pour le joueur %d", player + 1);
  //   positionDemande = 0;
  //     if (player + 1 == 1) {
  //       send_to_client_text(client1, "C'est votre tour, donnez le numéro de la "
  //                                   "case que vous voulez jouer -> ");
  //       send_to_client_text(client2, "Ce n'est pas votre tour...\n");
  //       read_client(client1->sock, buffer);
  //     } else {
  //       send_to_client_text(client1, "Ce n'est pas votre tour...\n");
  //       send_to_client_text(client2, "C'est votre tour, donnez le numéro de la "
  //                                   "case que vous voulez jouer -> ");
  //       read_client(client2->sock, buffer);
  //     }
  //   sscanf(buffer, "%d", &positionDemande);
  //   while (appliquerCoup(player, positionDemande, jeu)) {
  //     if (player + 1 == 1) {
  //       send_to_client_text(
  //           client1,
  //           "Le coup n'est pas autorisé, restez focus ! Nouvelle chance -> ");
  //       read_client(client1->sock, buffer);
  //     } else {
  //       send_to_client_text(
  //           client2,
  //           "Le coup n'est pas autorisé, restez focus ! Nouvelle chance -> ");
  //       read_client(client2->sock, buffer);
  //     }
  //     sscanf(buffer, "%d", &positionDemande);
  //   };
  //   afficher_jeu(*jeu, client1, client2);
  //   player = (player + 1) % 2;
  //}
}

static void handle_game_move(Client* sender,char* buffer){
  printf("%s joue : %s\n",sender->player->name, buffer);
}

static void send_welcome_message(Client *client, int first_co) {
  char message[BUF_SIZE];
  message[0] = 0;
  sprintf(message,
          "Bonjour %s ! Bienvenue sur le meilleur seveur de awale. Içi, une "
          "seule règle : s'amuser !",
          client->player->name);
  if (first_co == 1) {
    strncat(message,
            "\nC'est votre première fois içi, voulez vous vous écrire une bio? "
            "(Oui : Y, Non : N)",
            sizeof(message) - strlen(message) - 1);
  }
  send_to_client_text(client, message);
}

static Client *is_client_unocupied(Client *clients, char *client, int actual) {
  int position = 0;
  Client clientTmp;
  for (position = 0; position < actual; position++) {
    clientTmp = clients[position];
    if (strcmp(client, clientTmp.player->name) == 0 &&
        clientTmp.player->status == Unocupied) {
      return &clients[position];
    }
  }
  return 0;
}

static void afficher_clients(int taille, Client *clients) {
  for (int i = 0; i < taille; i++) {
    printf("nom : %s, status : %d\n", clients[i].player->name,
           clients[i].player->status);
  }
}

static void afficher_players(int taille, Player **players) {
  for (int i = 0; i < taille; i++) {
    printf("nom : %s, id : %d\n", players[i]->name, players[i]->id);
  }
}

static Player *find_player_by_name(char *buffer, Player **players,
                                   int nb_players) {
  for (int i = 0; i < nb_players; i++) {
    if (strcmp(buffer, players[i]->name) == 0) {
      return players[i];
    }
  }
  return 0;
}

static Client *find_client_by_socket(SOCKET sock, Client *clients,
                                   int nb_client) {
  for (int i = 0; i < nb_client; i++) {
    if (sock == clients[i].sock) {
      return &clients[i];
    }
  }
  return 0;
}

static void afficher_jeu(jeu_t jeu, Client *client1, Client *client2) {
  char message[1024];
  char tmp[32];
  message[0] = 0;
  strncpy(message, "", sizeof(message) - strlen(message) - 1);
  for (int i = 0; i < 6; i++) {
    snprintf(tmp, sizeof tmp, "%d", jeu.plateau[i]);
    strncat(message, tmp, sizeof(message) - strlen(message) - 1);
    strncat(message, " ", sizeof(message) - strlen(message) - 1);
  }
  strncat(message, "\n", sizeof(message) - strlen(message) - 1);
  for (int i = 11; i > 5; i--) {
    snprintf(tmp, sizeof tmp, "%d", jeu.plateau[i]);
    strncat(message, tmp, sizeof(message) - strlen(message) - 1);
    strncat(message, " ", sizeof(message) - strlen(message) - 1);
  }
  strncat(message, "\n", sizeof(message) - strlen(message) - 1);
  strncat(message, "Score de ", sizeof(message) - strlen(message) - 1);
  strncat(message, client1->player->name,
          sizeof(message) - strlen(message) - 1);
  strncat(message, " : ", sizeof(message) - strlen(message) - 1);
  snprintf(tmp, sizeof tmp, "%d", jeu.j1Score);
  strncat(message, tmp, sizeof(message) - strlen(message) - 1);
  strncat(message, "\nScore de ", sizeof(message) - strlen(message) - 1);
  strncat(message, client2->player->name,
          sizeof(message) - strlen(message) - 1);
  strncat(message, " : ", sizeof(message) - strlen(message) - 1);
  snprintf(tmp, sizeof tmp, "%d", jeu.j2Score);
  strncat(message, tmp, sizeof(message) - strlen(message) - 1);
  strncat(message, "\n", sizeof(message) - strlen(message) - 1);
  send_to_client_text(client1, message);
  send_to_client_text(client2, message);
}

int main(int argc, char **argv) {
  init();

  app();

  end();

  return EXIT_SUCCESS;
}
