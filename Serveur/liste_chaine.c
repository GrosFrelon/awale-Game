#include "liste_chaine.h"
#include <stdlib.h>

Game *find_game_with_player(Player *player, game_node **list_games) {
  game_node *game_element = *list_games;
  while (game_element != NULL &&
         !(game_element->game->playing == 1 &&
           (game_element->game->player1->id == player->id ||
            game_element->game->player2->id == player->id))) {
    game_element = game_element->next;
  }
  if (game_element != NULL) {
    return game_element->game;
  }
  return NULL;
}

void create_game(Game *game, game_node **list_games) {
  game_node *node = malloc(sizeof(game_node));
  if (!node)
    return; /* ou gérer l'erreur */
  node->game = game;
  node->next = *list_games;
  *list_games = node;
}

void delete_game(Game *game, game_node **list_games) {
  game_node **current = list_games;
  while (*current) {
    if ((*current)->game == game) {
      game_node *to_free = *current;
      *current = to_free->next;
      free(to_free);
      /* ne pas avancer current : *current est déjà le suivant */
    } else {
      current = &(*current)->next;
    }
  }
}