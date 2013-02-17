/*
 *    Copyright (c) 2012 maximilian attems <attems@fias.uni-frankfurt.de>
 *    GNU General Public License (GPLv3)
 */
#ifndef SRC_INCLUDE_BOX_H_
#define SRC_INCLUDE_BOX_H_

/* Cube edge length */
extern float A;

/* temporal time step */
extern float EPS;

/* number of steps */
extern int STEPS;

/* Temperature of the Boltzmann distribution for thermal initialization */
extern float temperature;

/* Debug runs generate more output */
extern bool verbose;

/* Default random seed */
extern unsigned int seedp;

/* Compile time debug info */
#ifdef DEBUG
# define printd printf
#else
# define printd(...) ((void)0)
#endif

#endif  // SRC_INCLUDE_BOX_H_
