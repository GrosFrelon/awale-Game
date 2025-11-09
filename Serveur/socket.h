#include "awale.h"
#include "client.h"
#include <errno.h>
// #include <sys/socket.h>

#include "types.h"
#include <arpa/inet.h>
#include <netdb.h> /* gethostbyname */
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> /* close */

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)

int send_exact(SOCKET sock, const void *buffer, size_t n);

void send_to_client_text(Client *client, const char *message);
void send_to_client_game(Client *client, jeu_t *game);
void send_to_client_player(Client *client, Player *player);
void send_to_client_clear(Client *client);

// Fonctions du prof

int init_connection(void);
int read_client(SOCKET sock, char *buffer);
void end_connection(int sock);
void clear_clients(Client *clients, int actual);
