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

  // on travaille sur une copie du plateau pour verifier que le coup est bon

  int copie_plateau[12];
  for (int i = 0; i < 12; i++) {
    copie_plateau[i] = jeu->plateau[i];
  }

  // On enlève les graines
  nbGrainesRecuperees = copie_plateau[position];
  if (nbGrainesRecuperees == 0) {
    return 2;
  }
  copie_plateau[position] = 0;
  int points_gagnes_1 = 0;
  int points_gagnes_2 = 0;

  // On sème les graines
  // nbGraineRecup%12 pour compenser les sauts de la case ou l'on a recup les
  // graines
  posDemarageSemage = position + jeu->rotation;
  for (int i = posDemarageSemage;
       i < (nbGrainesRecuperees + nbGrainesRecuperees / 12 +
            posDemarageSemage) &&
       i > (-1 * (nbGrainesRecuperees + nbGrainesRecuperees / 12 -
                  posDemarageSemage));
       i = i + jeu->rotation) {
    indiceParcours =
        ((i % 12) + 12) % 12; // toujours un nombre positif entre 0 et 12
    if (indiceParcours != position) {
      copie_plateau[indiceParcours]++;
    }
  }

  // Récupère les graines
  while ((copie_plateau[indiceParcours] == 2 ||
          copie_plateau[indiceParcours] == 3) &&
         positionCampAdverse(indiceParcours, numeroJoueur)) {
    if (numeroJoueur == 1) {
      points_gagnes_1 += copie_plateau[indiceParcours];
    } else {
      points_gagnes_2 += copie_plateau[indiceParcours];
    }
    copie_plateau[indiceParcours] = 0;
    indiceParcours -= jeu->rotation; // retourne en arrière du sens de rotation
    // pour recup les graines
  }

  if (numeroJoueur == 1) {
    int graines2 = 0;
    for (int i = 6; i < 12; i++) {
      graines2 += copie_plateau[i];
    }
    if (graines2 == 0) {
      return 3;
    }
  } else {
    int graines1 = 0;
    for (int i = 0; i < 6; i++) {
      graines1 += copie_plateau[i];
    }
    if (graines1 == 0) {
      return 3;
    }
  }

  for (int i = 0; i < 12; i++) {
    jeu->plateau[i] = copie_plateau[i];
  }

  jeu->j1Score += points_gagnes_1;
  jeu->j2Score += points_gagnes_2;
  return 0;
}

// Return 1 si on est dans le camp adverse
int positionCampAdverse(int position, int numeroJoueur) {
  if (position < 0 || position > 11)
    return 0;
  if (numeroJoueur == 1) { // camp adverse = 6..11
    return position >= 6 && position <= 11;
  } else if (numeroJoueur == 2) { // camp adverse = 0..5
    return position >= 0 && position <= 5;
  }
  return 0;
}