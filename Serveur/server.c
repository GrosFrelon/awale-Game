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
  /* an array for all clients and players */
  Client clients[MAX_CLIENTS];
  Player **players = malloc(taille_liste_player * sizeof(Player *));
  game_node *list_games = NULL;

  int id = load_players(&players, &nombre_player, &taille_liste_player) + 1;
  int game_id = 0;

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
      save_players(players, nombre_player);
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
      int player_already_co = 0;
      Player *p = find_player_by_name(buffer, players, nombre_player);
      if (p != 0) {
        player_already_co = player_already_connected(p, clients, actual);
      } else { // Si le nom est n'est associé à aucun players
        p = initialize_player(buffer, &players, &nombre_player,
                              &taille_liste_player, &id);
        fist_co = 1;
      }
      Client c;
      if (!player_already_co) {
        c.sock = csock;
        c.player = p;
      } else {
        c.sock = csock;
        c.player = 0;
      }
      clients[actual] = c;
      actual++;
      send_welcome_message(&c, fist_co, player_already_co);
    } else {
      int i = 0;
      for (i = 0; i < actual; i++) {
        /* a client is talking */
        if (FD_ISSET(clients[i].sock, &rdfs)) {
          Client client = clients[i];
          int c = read_client(clients[i].sock, buffer);
          /* client disconnected */
          if (c == 0) {
            // gérer déconnexion pendant partie

            Player *player = clients[i].player;

            if (player != NULL) {
              if (player->status == Ingame) {
                Client *opponent = find_client_by_socket(
                    player->opponent_socket, clients, actual);
                if (opponent) {
                  send_to_client_text(
                      opponent, "\nVotre adversaire s'est déconnecté. Vous "
                                "gagnez par forfait.\n");
                  opponent->player->status = Unocupied;
                  opponent->player->opponent_socket = -1;

                  player->gamesPlayed++;
                  opponent->player->gamesPlayed++;
                  opponent->player->gamesWon++;

                  Game *game = find_game_with_player(player, &list_games);
                  if (game) {
                    int nb_watchers = game->nb_watchers;
                    for (int i = 0; i < nb_watchers; i++) {
                      char message[BUF_SIZE];
                      sprintf(message,
                              "\n%s s'est déconnecté. Vicoire de %s !\n",
                              player->name, opponent->player->name);
                      send_to_client_text(game->watchers[i], message);
                      quit_watching(game->watchers[i]);
                    }
                    delete_game(game, &list_games);
                  }
                }
                player->status = Unocupied;
              }
            }

            closesocket(clients[i].sock);
            remove_client(clients, i, &actual);

          } else {
            Client *sender = &clients[i];
            Client *opponent;
            if (sender->player == 0) { // Si on a pas de player associé
              connect_client_to_player(sender, buffer, clients, &players,
                                       &nombre_player, actual,
                                       &taille_liste_player, &id);
            } else {
              switch (sender->player->status) {
              case Waiting_for_opponent:
              case Challenge_pending:
                opponent = find_client_by_socket(
                    sender->player->opponent_socket, clients, actual);
                handle_challenge_response(sender, opponent, buffer, clients,
                                          &list_games, &game_id);
                break;
              case Unocupied:
                analyse_command(&clients[i], buffer, clients, actual,
                                &list_games, players, nombre_player);
                break;
              case Ingame:
                opponent = find_client_by_socket(
                    sender->player->opponent_socket, clients, actual);
                handle_game_move(sender, opponent, buffer, &list_games);
                break;
              case Waiting_for_conf_bio:
                handle_bio_response(sender, buffer);
                break;
              case Writting_bio:
                handle_writting_bio(sender, buffer);
                break;
              case Watching:
                quit_watching(sender);
                break;
              default:
                break;
              }
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

static Player *initialize_player(char *name, Player ***players,
                                 int *nombre_player, int *taille_liste_player,
                                 int *id) {
  Player *p = malloc(sizeof(Player));
  p->elo = 0;
  p->gamesPlayed = 0;
  p->gamesWon = 0;
  p->status = Unocupied;
  p->id = (*id)++;
  p->opponent_socket = -1;
  p->active_game = NULL;
  strncpy(p->name, name, NAME_SIZE);
  strncpy(p->bio, " ", BIO_SIZE); // Pour pouvoir parser plus simplement la save
  add_player(players, p, nombre_player, taille_liste_player);
  return p;
}

static void connect_client_to_player(Client *sender, char *name,
                                     Client *clients, Player ***players,
                                     int *nombre_player, int nombre_client,
                                     int *taille_liste_player, int *id) {
  int fist_co = 0;
  int player_already_co = 0;
  Player *p = find_player_by_name(name, *players, *nombre_player);
  if (p != 0) {
    player_already_co = player_already_connected(p, clients, nombre_client);
  }
  if (p == 0) { // Si le nom est n'est associé à aucun players
    p = initialize_player(name, players, nombre_player, taille_liste_player,
                          id);
    fist_co = 1;
  }
  if (!player_already_co) {
    sender->player = p;
  }
  send_welcome_message(sender, fist_co, player_already_co);
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

  strncpy(message, "List of unoccupied players:\n",
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

  strncat(message, "List of in game players:\n",
          sizeof(message) - strlen(message) - 1);

  for (i = 0; i < actual; i++) {
    if (clients[i].player->status == Ingame &&
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
                            int actual, game_node **list_games,
                            Player **players, int nombre_player) {
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
    } else if (strncmp(buffer, "/spectate", 9) == 0) {
      char receiver_name[BUF_SIZE];
      if (sscanf(buffer + 9, " %s", receiver_name) != 1) {
        send_to_client_text(sender, "Usage: /spectate <name>\n");
        return;
      }
      spectate(sender, receiver_name, clients, actual, list_games);
    } else if (strncmp(buffer, "/me", 3) == 0) {
      send_to_client_player(sender, sender->player);
    } else if (strncmp(buffer, "/player", 7) == 0) {
      char player_name[BUF_SIZE];
      if (sscanf(buffer + 7, " %s", player_name) != 1) {
        send_to_client_text(sender, "Usage: /player <name>\n");
        return;
      }
      Player *found_player =
          find_player_by_name(player_name, players, nombre_player);
      if (found_player == 0) {
        send_to_client_text(sender, "404 Player not found\n");
      } else {
        send_to_client_player(sender, found_player);
      }
    } else if (strncmp(buffer, "/help", 3) == 0) {
      send_to_client_text(
          sender,
          "Awale Game by Raphaël LETOURNEUR and Alois PINTO DE SILVA -- "
          "WINNEFELD\n\n"
          "Available commands : "
          "\n\t- /me : display information about yourself"
          "\n\t- /users : display a list of all users"
          "\n\t- /player <name> : display information about a player"
          "\n\t- /challenge <name> : challenge player to a game of awale"
          "\n\t- /spectate <name> : spectate player's current game of awale"
          "\n\n");
    } else {
      send_to_client_text(sender,
                          "bad command, type /help for help on commands\n");
    }
  }
}

static void send_request_challenge(Client *sender, char *receiver,
                                   Client *clients, int actual,
                                   game_node **list_games) {
  Client *pClient = is_client_unocupied(clients, receiver, actual);
  char message[BUF_SIZE];
  message[0] = 0;
  if (pClient == 0) {
    sprintf(message, "Receiver not found : %s\n", receiver);
    send_to_client_text(sender, message);
  } else if (pClient->sock == sender->sock) {
    sprintf(message, "You can't play against yourself\n");
    send_to_client_text(sender, message);
  } else { // Comment on différencie les clients? par le sock ou le nom. En vrai
    // pareil parcqu'on retrouve la personne avec son nom donc nom unique
    sprintf(
        message,
        "Challenge request send to : %s (Press N to cancel the invitation)\n",
        pClient->player->name);
    send_to_client_text(sender, message);

    message[0] = 0;
    sprintf(message, "%s challenges you \nPress Y to accept, N to reject\n",
            sender->player->name);
    send_to_client_text(pClient, message);

    pClient->player->status = Challenge_pending;
    sender->player->status = Waiting_for_opponent;

    sender->player->opponent_socket = pClient->sock;
    pClient->player->opponent_socket = sender->sock;
  }
}

static void handle_challenge_response(Client *sender, Client *opponent,
                                      char *buffer, Client *clients,
                                      game_node **list_games, int *game_id) {

  char message[BUF_SIZE];
  message[0] = 0;

  if ((strcmp(buffer, "Y") == 0 || strcmp(buffer, "y") == 0) &&
      sender->player->status == Challenge_pending) {
    start_game(opponent, sender, list_games, game_id);
  } else if (strcmp(buffer, "N") == 0 || strcmp(buffer, "n") == 0) {
    message[0] = 0;
    sprintf(message, "%s refused the challenge\n", sender->player->name);
    send_to_client_text(opponent, message);
    sprintf(message, "You refused the challenge\n");
    send_to_client_text(sender, message);
    opponent->player->status = Unocupied;
    sender->player->status = Unocupied;
    sender->player->opponent_socket = -1;
    opponent->player->opponent_socket = -1;
  }
}

static void start_game(Client *client1, Client *client2, game_node **list_games,
                       int *game_id) {
  char message[BUF_SIZE];
  char buffer[BUF_SIZE];
  message[0] = 0;
  sprintf(message, "New game started between %s (Player 1) and %s (Player 2)",
          client1->player->name, client2->player->name);
  send_to_client_text(client1, message);
  send_to_client_text(client2, message);
  client1->player->status = Ingame;
  client2->player->status = Ingame;

  jeu_t *jeu = initGame(-1, client1->player->name, client2->player->name);

  Game *game = malloc(sizeof(Game));
  game->game_id = (*game_id)++;
  game->jeu = *jeu;
  game->player1 = client1->player;
  game->player2 = client2->player;
  game->playing = 1;
  game->winner = 0;
  game->nb_watchers = 0;
  game->watchers_list_size = 0;
  game->watchers = NULL;

  free(jeu);

  create_game(game, list_games);

  send_to_client_clear(client1);
  send_to_client_clear(client2);

  send_to_client_game(client1, &game->jeu);
  send_to_client_game(client2, &game->jeu);
  for (int i = 0; i < game->nb_watchers; i++) {
    send_to_client_clear(game->watchers[i]);
    sprintf(message, "You are spectating. Send any key to stop.\n\n");
    send_to_client_text(game->watchers[i], message);
    send_to_client_game(game->watchers[i], &game->jeu);
  }
  // afficher(*jeu);

  if (game->jeu.active_player == 1) {
    send_to_client_text(client1, "C'est votre tour, donnez le numéro de la "
                                 "case que vous voulez jouer -> ");
    send_to_client_text(client2, "Ce n'est pas votre tour...\n");
  } else {
    send_to_client_text(client1, "Ce n'est pas votre tour...\n");
    send_to_client_text(client2, "C'est votre tour, donnez le numéro de la "
                                 "case que vous voulez jouer -> ");
  }
}

static void handle_game_move(Client *sender, Client *opponent, char *buffer,
                             game_node **list_games) {
  Game *game = find_game_with_player(sender->player, list_games);

  if ((game->jeu.active_player == 1 &&
       game->player1->id == sender->player->id) ||
      (game->jeu.active_player == 2 &&
       game->player2->id == sender->player->id)) {

    int positionDemande = 0;
    sscanf(buffer, "%d", &positionDemande);

    if (appliquerCoup(game->jeu.active_player, positionDemande, &game->jeu) ==
        0) {
      // if (game->jeu.j1Score >= 24 || game->jeu.j2Score >= 24) {
      if (game->jeu.j1Score >= 2 || game->jeu.j2Score >= 2) {
        game->playing = 0;
        game->winner = (game->jeu.j1Score > game->jeu.j2Score) ? 1 : 2;

        char message[BUF_SIZE];
        sprintf(message, "Fin de partie ! Gagnant : %s\n",
                game->winner == 1 ? game->player1->name : game->player2->name);
        send_to_client_text(sender, message);
        send_to_client_text(opponent, message);

        int nb_watchers = game->nb_watchers;
        for (int i = 0; i < nb_watchers; i++) {
          char message[BUF_SIZE];
          sprintf(message, "Fin de partie. Vicoire de %s !\n",
                  game->winner == 1 ? game->player1->name
                                    : game->player2->name);
          send_to_client_text(game->watchers[i], message);
          quit_watching(game->watchers[i]);
        }

        if (game->winner == 1) {
          game->player1->gamesWon++;
        } else {
          game->player2->gamesWon++;
        }

        sender->player->gamesPlayed++;
        opponent->player->gamesPlayed++;
        sender->player->status = Unocupied;
        opponent->player->status = Unocupied;
        sender->player->opponent_socket = -1;
        opponent->player->opponent_socket = -1;

        delete_game(game, list_games);
      } else {

        game->jeu.active_player = (game->jeu.active_player == 1) ? 2 : 1;
        // printf("%s joue : %s\n", sender->player->name, buffer);

        send_to_client_clear(sender);
        send_to_client_clear(opponent);

        char message[BUF_SIZE];

        send_to_client_game(sender, &game->jeu);
        send_to_client_game(opponent, &game->jeu);
        for (int i = 0; i < game->nb_watchers; i++) {
          send_to_client_clear(game->watchers[i]);
          sprintf(message, "You are spectating. Send any key to stop.\n\n");
          send_to_client_text(game->watchers[i], message);
          send_to_client_game(game->watchers[i], &game->jeu);
        }
        // afficher(game->jeu);

        send_to_client_text(opponent,
                            "C'est votre tour, donnez le numéro de la "
                            "case que vous voulez jouer -> ");
        send_to_client_text(sender, "Ce n'est pas votre tour...\n");
      }
    } else {
      send_to_client_text(
          sender,
          "Le coup n'est pas autorisé, restez focus ! Nouvelle chance -> ");
    }
  } else {
    send_to_client_text(sender, "Ce n'est pas à vous de jouer...\n");
  };
}

static void send_welcome_message(Client *client, int first_co,
                                 int player_already_co) {
  char message[BUF_SIZE];
  message[0] = 0;
  if (!player_already_co) {
    sprintf(message,
            "Bonjour %s ! Bienvenue sur le meilleur seveur de awale. Içi, une "
            "seule règle : s'amuser !\n",
            client->player->name);
    if (first_co == 1) {
      strncat(
          message,
          "\nC'est votre première fois içi, voulez vous vous écrire une bio? "
          "(Oui : Y, Non : N)",
          sizeof(message) - strlen(message) - 1);
      client->player->status = Waiting_for_conf_bio;
    }
  } else {
    strcpy(message, "Ce player est déjà utilisé par un autre client...\nTester "
                    "une prochaine fois ou avec un autre player : ");
  }
  send_to_client_text(client, message);
}

static void handle_bio_response(Client *sender, char *buffer) {
  if (strcmp(buffer, "Y") == 0 || strcmp(buffer, "y") == 0) {
    sender->player->status = Writting_bio;
    send_to_client_text(sender, "Votre bio : ");
  } else if ((strcmp(buffer, "N") == 0 || strcmp(buffer, "n") == 0)) {
    sender->player->status = Unocupied;
    send_to_client_text(sender, "Vous n'avez pas de bio ;(\n");
  }
}

static void handle_writting_bio(Client *sender, char *buffer) {
  size_t len = strcspn(buffer, "\r\n"); // longueur sans fin de ligne

  if (len >= sizeof(sender->player->bio)) {
    len = sizeof(sender->player->bio) - 1; // éviter le dépassement
  }

  memcpy(sender->player->bio, buffer, len);
  sender->player->bio[len] = '\0';

  sender->player->status = Unocupied;

  send_to_client_text(sender, "Bio enregistrée.\n");
}

static void quit_watching(Client *sender) {

  Game *game = sender->player->active_game;

  if (game != NULL) {
    for (int i = 0; i < game->nb_watchers; i++) {
      if (game->watchers[i] == sender) {
        // tableau donc ajustement index
        for (int j = i; j < game->nb_watchers - 1; j++) {
          game->watchers[j] = game->watchers[j + 1];
        }
        game->nb_watchers--;
        break;
      }
    }
    sender->player->active_game = NULL;
  }

  sender->player->status = Unocupied;

  send_to_client_text(sender, "No longer watching the game.\n");
}

static Client *find_client_by_name(Client *clients, char *client, int actual) {
  int position = 0;
  Client clientTmp;
  for (position = 0; position < actual; position++) {
    clientTmp = clients[position];
    if (strcmp(client, clientTmp.player->name) == 0) {
      return &clients[position];
    }
  }
  return 0;
}

static void spectate(Client *sender, char *player_name, Client *clients,
                     int actual, game_node **list_games) {
  Client *pClient = find_client_by_name(clients, player_name, actual);
  char message[BUF_SIZE];
  if (pClient == 0) {
    sprintf(message, "Player not found : %s\n", player_name);
    send_to_client_text(sender, message);
  } else if (pClient->player->status != Ingame) {
    sprintf(message,
            "Player %s is not in game. Check /users for a list of in game "
            "players.\n",
            player_name);
    send_to_client_text(sender, message);
  } else {
    Game *game = find_game_with_player(pClient->player, list_games);

    sender->player->status = Watching;

    add_watcher(game, sender);

    send_to_client_clear(sender);

    sprintf(message,
            "You are now spectating %s's game. Send any key to stop.\n\n",
            player_name);
    send_to_client_text(sender, message);
    send_to_client_game(sender, &game->jeu);
  }
}

static void add_watcher(Game *game, Client *client) {
  if (game == NULL) {
    return;
  }
  if (game->nb_watchers >= game->watchers_list_size) {
    int new_cap = game->watchers_list_size + 3;
    Client **new_list = realloc((game->watchers), new_cap * sizeof(Client *));
    game->watchers = new_list;
    game->watchers_list_size = new_cap;
  }
  game->watchers[game->nb_watchers] = client;
  game->nb_watchers++;
  client->player->active_game = game;
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

static int player_already_connected(Player *p, Client *clients, int actual) {
  for (int i = 0; i < actual; i++) {
    if (p == clients[i].player) {
      return 1;
    }
  }
  return 0;
}

static void afficher_clients(int taille, Client *clients) {
  for (int i = 0; i < taille; i++) {
    printf("Client : socket : %d\n", clients[i].sock);
  }
}

static void afficher_players(int taille, Player **players) {
  for (int i = 0; i < taille; i++) {
    printf("Player : nom : %s, id : %d\n", players[i]->name, players[i]->id);
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

static void save_players(Player **players, int nombre_player) {
  Player *player_tmp;
  FILE *file = fopen(SAVE_FILE, "w");
  for (int i = 0; i < nombre_player; i++) {
    player_tmp = players[i];
    fprintf(file, "%d;", player_tmp->id);
    fprintf(file, "%s;", player_tmp->name);
    fprintf(file, "%s;", player_tmp->bio);
    fprintf(file, "%d;", player_tmp->gamesPlayed);
    fprintf(file, "%d;", player_tmp->gamesWon);
    fprintf(file, "%d;\n", player_tmp->elo);
  }
  fclose(file);
}

static int load_players(Player ***players, int *nombre_player,
                        int *taille_liste) {
  FILE *file = fopen(SAVE_FILE, "r");
  if (file == NULL) {
    return 1;
  }

  int id = -1; // Si on charge aucun player : aucun players du tout donc
               // prochain player -> id=0
  int maximum_line_length = sizeof(Player);
  char lineBuffer[maximum_line_length];

  while (fgets(lineBuffer, maximum_line_length, file)) {
    lineBuffer[strcspn(lineBuffer, "\r\n")] = '\0';
    if (lineBuffer[0] == '\0') {
      continue;
    }

    char *id_str = strtok(lineBuffer, ";");
    char *name = strtok(NULL, ";");
    char *bio = strtok(NULL, ";");
    char *played_str = strtok(NULL, ";");
    char *won_str = strtok(NULL, ";");
    char *elo_str = strtok(NULL, ";");

    Player *p = (Player *)malloc(sizeof(Player));
    strncpy(p->name, name, NAME_SIZE);
    strncpy(p->bio, bio, BIO_SIZE);
    p->gamesPlayed = atoi(played_str);
    p->gamesWon = atoi(won_str);
    p->elo = atoi(elo_str);
    p->id = atoi(id_str);
    p->status = Unocupied;
    p->opponent_socket = -1;

    add_player(players, p, nombre_player, taille_liste);

    id = p->id;
  }
  // afficher_players(*nombre_player, *players);
  fclose(file);
  return id;
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
