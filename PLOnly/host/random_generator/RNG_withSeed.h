//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: RNG_withSeed.h
//
// MATLAB Coder version            : 5.4
// C/C++ source code generated on  : 01-Sep-2023 10:45:31
//

#ifndef RNG_WITHSEED_H
#define RNG_WITHSEED_H

// Include Files
// #include "rtwtypes.h"
#include <cstddef>
#include <cstdlib>
#include <stdbool.h>

#define MAX_uint32_T 4294967295
// Function Declarations
double RNG_withSeed(double init, double seed_in);

void RNG_withSeed_initialize();

void RNG_withSeed_terminate();

#endif
//
// File trailer for RNG_withSeed.h
//
// [EOF]
//
