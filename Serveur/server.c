#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "liste_chaine.h"
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
  Player **players = malloc(taille_liste_player * sizeof(Player*));

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

      //Player* p = find_player_by_name(buffer)  //TODO
      
      Player* p = malloc(sizeof(Player));
      p->elo = 0;
      p->gamePlayed = 0;
      p->gamesWon = 0;
      p->status = Unocupied;
      p->id = id++;
      strncpy(p->name, buffer, BUF_SIZE - 1);
      
      Client c = {.sock = csock, .player = p};
      
      clients[actual] = c;
      add_player(&players, p, &nombre_player, &taille_liste_player);
      actual++;
      send_welcome_message(c);
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
            analyse_command(&clients[i], buffer, clients, actual);
          }
          break;
        }
      }
    }
  }

  clear_clients(clients, actual);
  end_connection(sock);
}

static void clear_clients(Client *clients, int actual) {
  int i = 0;
  for (i = 0; i < actual; i++) {
    closesocket(clients[i].sock);
  }
}

static void remove_client(Client *clients, int to_remove, int *actual) {
  /* we remove the client in the array */
  memmove(clients + to_remove, clients + to_remove + 1,
          (*actual - to_remove - 1) * sizeof(Client));
  /* number client - 1 */
  (*actual)--;
}

static void add_player(Player*** players, Player* player, int* nombre_player, int* taille_liste){
  if(*nombre_player>=*taille_liste){
    int new_cap = *taille_liste + 10;
    Player **new_arr = realloc(*players, new_cap * sizeof(Player*));
    *players = new_arr;
    *taille_liste = new_cap;
  }
  (*players)[*nombre_player] = player;
  (*nombre_player)++;
}

static void send_message_to_all_clients(Client *clients, Client sender,
                                        int actual, const char *buffer,
                                        char from_server) {
  int i = 0;
  char message[BUF_SIZE];
  message[0] = 0;
  for (i = 0; i < actual; i++) {
    /* we don't send message to the sender */
    if (sender.sock != clients[i].sock) {
      if (from_server == 0) {
        strncpy(message, sender.player->name, BUF_SIZE - 1);
        strncat(message, " : ", sizeof message - strlen(message) - 1);
      }
      strncat(message, buffer, sizeof message - strlen(message) - 1);
      write_client(clients[i].sock, message);
    }
  }
}

static void send_unoccupied_clients(Client *clients, Client sender,
                                    int actual) {
  int i = 0;
  char message[BUF_SIZE];
  message[0] = 0;

  strncpy(message, "List of online players unocupied:\n",
          sizeof(message) - strlen(message) - 1);

  for (i = 0; i < actual; i++) {
    if (clients[i].player->status == Unocupied && clients[i].sock != sender.sock) {
      strncat(message, "\t- ", sizeof(message) - strlen(message) - 1);
      strncat(message, clients[i].player->name, sizeof(message) - strlen(message) - 1);
      strncat(message, "\n", sizeof(message) - strlen(message) - 1);
    }
  }
  write_client(sender.sock, message);
}

static int init_connection(void) {
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

static void end_connection(int sock) { closesocket(sock); }

static int read_client(SOCKET sock, char *buffer) {
  int n = 0;

  if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
    perror("recv()");
    /* if recv error we disonnect the client */
    n = 0;
  }

  buffer[n] = 0;

  return n;
}

static void write_client(SOCKET sock, const char *buffer) {
  if (send(sock, buffer, strlen(buffer), 0) < 0) {
    perror("send()");
    exit(errno);
  }
}

static void analyse_command(Client* sender, const char *buffer, Client *clients,
                            int actual) {
  if (buffer[0] == '/') {
    if (strncmp(buffer, "/users", 6) == 0) {
      send_unoccupied_clients(clients, *sender, actual);
    }
    else if (strncmp(buffer, "/challenge", 10) == 0) {
      char receiver_name[BUF_SIZE];
      if(sscanf(buffer+10, " %s", receiver_name) != 1){
        write_client( sender->sock, "Usage: /challenge <name>\n");
        return;
      }
      send_request_challenge(sender, receiver_name, clients, actual);
    }
    else{
      write_client(sender->sock, "bad command");
    }
  }
}

static void send_request_challenge(Client* sender, char* receiver, Client* clients, int actual){
  Client* pClient = is_client_unocupied(clients, receiver, actual);
  char message[BUF_SIZE];
  message[0] = 0;
  if(pClient==0){
    strncpy(message, "Receiver not found : ", sizeof(message) - strlen(message) - 1);
    strncat(message, receiver, sizeof(message) - strlen(message) - 1);
    write_client(sender->sock, message);
  }
  else if (pClient->sock == sender->sock){
    strncpy(message, "You can't play against yourself", sizeof(message) - strlen(message) - 1);
    write_client(sender->sock, message);
  }
  else {   //Comment on différencie les clients? par le sock ou le nom. En vrai pareil parcqu'on retrouve la personne avec son nom donc nom unique
    strncpy(message, "Challenge request send to : ", sizeof(message) - strlen(message) - 1);
    strncat(message, pClient->player->name, sizeof(message) - strlen(message) - 1);
    write_client(sender->sock, message);

    message[0] = 0;
    strncpy(message, sender->player->name, sizeof(message) - strlen(message) - 1);
    strncat(message, " challenge you \n Press Y to accept, N to reject", sizeof(message) - strlen(message) - 1);
    write_client(pClient->sock, message);

    pClient->player->status = Waiting;
    sender->player->status = Waiting;
  }
  
}

static void send_welcome_message(Client client){
  char message[BUF_SIZE];
  message[0] = 0;
  strncpy(message, "Bonjour ", sizeof(message) - strlen(message) - 1);
  strncat(message, client.player->name, sizeof(message) - strlen(message) - 1);
  strncat(message, "! Bienvenue sur le meilleur seveur de awale. Içi, une seule règle, s'amuser!", sizeof(message) - strlen(message) - 1);
  write_client(client.sock, message);
}

static Client* is_client_unocupied(Client* clients, char* client, int actual){
  int position = 0;
  Client clientTmp;
  for(position=0; position<actual;position++){
    clientTmp = clients[position];
    if (strcmp(client,clientTmp.player->name)==0 && clientTmp.player->status == Unocupied){
      return &clients[position];
    }
  }
  return 0;
}

static void afficher_clients(int taille, Client* clients){
  for (int i=0; i<taille; i++){
    printf("nom : %s, status : %d\n", clients[i].player->name, clients[i].player->status);
  }
}

static void afficher_players(int taille, Player** players){
  for (int i=0; i<taille; i++){
    printf("nom : %s, id : %d\n", players[i]->name, players[i]->id);
  }
}


int main(int argc, char **argv) {
  init();

  app();

  end();

  return EXIT_SUCCESS;
}
