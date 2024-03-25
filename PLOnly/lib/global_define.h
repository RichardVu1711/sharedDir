#pragma once
#include <cstdio>
#include <string.h>
#include "Fixed_point_type.h"
#define NUM_PARTICLES	1024
#define	NUM_VAR	13
#define FS_C 0.1024683122
//#define AOASTD_SQRT 0.000149262535695487	// 0.7 degree
#define AOASTD_SQRT 0.00030462	//1 degree
#define TDOASTD_SQRT 49.0

#define PXX_AP_LEN (NUM_VAR*(NUM_VAR+1)/2)
#define N_AOA 3
#define N_TDOA 3
#define N_MEAS (N_AOA+N_TDOA)

extern const fixed_type sigma[NUM_VAR];
extern const fixed_type jacobian[NUM_VAR*NUM_VAR];
