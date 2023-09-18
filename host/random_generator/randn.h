/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 * File: randn.h
 *
 * MATLAB Coder version            : 5.2
 * C/C++ source code generated on  : 18-Sep-2021 23:05:32
 */

#pragma once

/* Include Files */




#include <stddef.h>
#include <stdlib.h>

#include <math.h>


#include "Random_init.h"




/* Function Declarations */
void randn(double *r, int init_flag,int seed);
double eml_rand_shr3cong(unsigned int e_state[2]);
void genrand_uint32_vector(unsigned int mt[625], unsigned int u[2]);


/*
 * File trailer for randn.h
 *
 * [EOF]
 */
