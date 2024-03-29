#pragma once
#include "Fixed_point_type.h"
#include "hls_stream.h"
//using namespace hls;
// consider about padding in here where it is required the total of bytes is a number of power of two
typedef struct Mat_S{
	fixed_type entries[NUM_VAR*NUM_VAR];
    int row;
	int col;
} Mat_S;

typedef struct Mat{
	fixed_type entries[NUM_VAR*NUM_PARTICLES];
    int row;
	int col;
} Mat;


typedef hls::stream <fixed_type> fp_str;
