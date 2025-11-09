#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "awale.h"

// Joueur 1-> associé au numéro 0
// Joueur 2 -> associé au numéro 1
// int main() {
//   jeu_t *jeu = initGame(1);
//   afficher(*jeu);

//   srand(time(NULL));
//   int player = (rand() % 2);
//   int positionDemande = 0;

//   while (jeu->j1Score < 24 || jeu->j2Score < 24) {
//     printf("coup pour le joueur %d", player + 1);
//     scanf("%d", &positionDemande);
//     while (appliquerCoup(player, positionDemande, jeu)) {
//       printf("reste focus %d", player + 1);
//       scanf("%d", &positionDemande);
//     };
//     afficher(*jeu);
//     player = (player + 1) % 2;

//     printf("coup pour le joueur %d", player + 1);
//     scanf("%d", &positionDemande);
//     while (appliquerCoup(player, positionDemande, jeu)) {
//       printf("reste focus %d", player + 1);
//       scanf("%d", &positionDemande);
//     };
//     afficher(*jeu);
//     player = (player + 1) % 2;
//   }
// }

jeu_t *initGame(int rotation, char *namePlayer1, char *namePlayer2) {
  jeu_t *jeu = malloc(sizeof(jeu_t));
  srand(time(NULL));
  for (int i = 0; i < 12; i++) {
    jeu->plateau[i] = 4;
  }
  jeu->j1Score = jeu->j2Score = 0;
  jeu->rotation = rotation;
  jeu->active_player = (rand() % 2) + 1;
  strcpy(jeu->j1Name, namePlayer1);
  strcpy(jeu->j2Name, namePlayer2);
  return jeu;
}

void afficher(jeu_t jeu) {
  for (int i = 0; i < 6; i++) {
    printf("%d ", jeu.plateau[i]);
  }
  printf("\n");
  for (int i = 11; i > 5; i--) {
    printf("%d ", jeu.plateau[i]);
  }
  printf("\n");
  printf("Score du J1 : %d \n", jeu.j1Score);
  printf("Score du J2 : %d \n", jeu.j2Score);
}

int appliquerCoup(int numeroJoueur, int position, jeu_t *jeu) {
  int nbGrainesRecuperees = 0;
  int posDemarageSemage = 0;
  int indiceParcours = 0;

  // On calcul la position du trou où enlever les graines
  if (position < 1 || position > 6) {
    return 1;
  }
  if (numeroJoueur == 1) {
    position = position - 1;
  }
  if (numeroJoueur == 2) {
    position = 12 - position;
  }

  // position = position initiale

  // On enlève les graines
  nbGrainesRecuperees = jeu->plateau[position];
  if (nbGrainesRecuperees == 0) {
    return 1;
  }
  jeu->plateau[position] = 0;

  // On sème les graines
  // nbGraineRecup%12 pour compenser les sauts de la case ou l'on a recup les
  // graines
  posDemarageSemage = position + jeu->rotation;
  for (int i = posDemarageSemage;
       i < (nbGrainesRecuperees + nbGrainesRecuperees / 12 + posDemarageSemage);
       i = i + jeu->rotation) {
    indiceParcours =
        ((i % 12) + 12) % 12; // toujours un nombre positif entre 0 et 12
    if (indiceParcours != position) {
      jeu->plateau[indiceParcours]++;
    }
  }

  // Récupère les graines
  while ((jeu->plateau[indiceParcours] == 2 ||
          jeu->plateau[indiceParcours] == 3) &&
         positionCampAdverse(indiceParcours, numeroJoueur)) {
    augmenterScore(numeroJoueur, jeu->plateau[indiceParcours], jeu);
    jeu->plateau[indiceParcours] = 0;
    indiceParcours -= jeu->rotation; // retourne en arrière du sens de rotation
                                     // pour recup les graines
  }
  return 0;
}

// Return 0 si on est dans le camp adverse
int positionCampAdverse(int position, int numeroJoueur) {
  return !(numeroJoueur * 6 <= position && position <= (numeroJoueur * 6 + 5));
}

void augmenterScore(int numeroJoueur, int graines, jeu_t *jeu) {
  if (numeroJoueur == 1) {
    jeu->j1Score += graines;
  }
  if (numeroJoueur == 2) {
    jeu->j2Score += graines;
  }
}