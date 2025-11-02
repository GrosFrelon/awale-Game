#ifndef SERVER_H
#define SERVER_H

// #ifdef WIN32

// #include <winsock2.h>

// #elif defined(linux)

#include <arpa/inet.h>
#include <netdb.h> /* gethostbyname */
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> /* close */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

// #else

// #error not defined for this platform

// #endif

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100

#define BUF_SIZE 1024

#include "client.h"

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client,
                                        int actual, const char *buffer,
                                        char from_server);
static void send_unoccupied_clients(Client *clients, Client client, int actual);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static void analyse_command(Client* client, const char *buffer, Client *clients,
                            int actual);
static void send_request_challenge(Client* sender, char* receiver, Client* clients, int actual);
static void send_welcome_message(Client client);
static Client* is_client_unocupied(Client* clients, char* client, int actual);
static void afficher_clients(int taille, Client* clients);

#endif /* guard */
