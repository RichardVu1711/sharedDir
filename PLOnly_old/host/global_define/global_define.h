#pragma once
#include <cstdio>

#include "Fixed_point_type.h"
#define NUM_PARTICLES	1024
#define	NUM_VAR	13
#define FS_C 0.1024683122
#define AOASTD_SQRT 0.00031
#define TDOASTD_SQRT 25.0

extern const fixed_type sigma[NUM_VAR];
extern const fixed_type jacobian[NUM_VAR*NUM_VAR];
