#include "liste_chaine.h"
#include <stdlib.h>

// void afficher() {
//   printf("affichage de la liste:\n");
//   elem *local_list = list;
//   while (1) {
//     if (list == NULL) {
//       printf("Liste vide.\n");
//       break;
//     } else if (local_list != NULL) {
//       printf("%d\n", local_list->value);
//       local_list = local_list->next;
//     } else {
//       break;
//     }
//   }
// }

void create_game(Game *game, game_node *list_games) {
  game_node *node = malloc(sizeof(game_node));
  node->game = game;
  node->next = list_games;
  list_games = node;
}

// void create_client(Client *client, client_node *list_clients) {
//   client_node *node = malloc(sizeof(client_node));
//   node->client = client;
//   node->next = list_clients;
//   list_clients = node;
// }

// void rechercher() {
//   int nb;
//   int indice = 0;
//   printf("tapez un nombre:\n");
//   scanf("%d", &nb);
//   printf("recherche de %d dans la liste:\n", nb);
//   elem *local_list = list;

//   while (1) {
//     if (local_list != NULL) {
//       if (local_list->value == nb) {
//         printf("trouvé : %d\n", indice);
//         break;
//       }
//       local_list = local_list->next;
//       indice++;
//     } else {
//       printf("pas trouvé\n");
//       break;
//     }
//   }
// }

void delete_game(Game *game, game_node *list_games) {
  if (list_games->game == game) // Première valeur
  {
    game_node *next = list_games->next;
    free(list_games);
    list_games = next;
  }

  game_node *local_list = list_games;

  while (1) {
    if (local_list != NULL && local_list->next != NULL) {
      if (local_list->next->game == game) {
        game_node *next_game_node = local_list->next->next;
        free(local_list->next);
        local_list->next = next_game_node;
      } else // si on a enlevé une case, il ne faut pas avancer car c'est la
             // suite de la liste qui s'est "rapprochée"
      {
        local_list = local_list->next;
      }
    } else {
      break;
    }
  }
}

// void delete_client(Client *client, client_node *list_clients) {
//   if (list_clients->client == client) // Première valeur
//   {
//     client_node *next = list_clients->next;
//     free(list_clients);
//     list_clients = next;
//   }

//   client_node *local_list = list_clients;

//   while (1) {
//     if (local_list != NULL && local_list->next != NULL) {
//       if (local_list->next->client == client) {
//         client_node *next_client_node = local_list->next->next;
//         free(local_list->next);
//         local_list->next = next_client_node;
//       } else // si on a enlevé une case, il ne faut pas avancer car c'est la
//              // suite de la liste qui s'est "rapprochée"
//       {
//         local_list = local_list->next;
//       }
//     } else {
//       break;
//     }
//   }
// }