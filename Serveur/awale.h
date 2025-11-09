#ifndef _AWALE_H
#define _AWALE_H

typedef struct {
  int plateau[12]; // [a, b, c, d, e, f, F, E, D, C, B, A]
  int rotation;    //+1 = sens horaire, -1 = sens anti-horaire
  int j1Score;
  int j2Score;
  int active_player; // 1 ou 2
} jeu_t;

jeu_t *initGame(int rotation);
void afficher(jeu_t jeu);
int appliquerCoup(int numeroJoueur, int position, jeu_t *jeu);
int positionCampAdverse(int position, int numeroJoueur);
void augmenterScore(int numeroJoueur, int graines, jeu_t *jeu);

#endif