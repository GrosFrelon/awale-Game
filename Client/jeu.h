#ifndef _JEU_H
#define _JEU_H

typedef struct {
  int plateau[12]; // [a, b, c, d, e, f, F, E, D, C, B, A]
  int rotation;    //+1 = sens horaire, -1 = sens anti-horaire
  int j1Score;
  int j2Score;
} jeu_t;

#endif