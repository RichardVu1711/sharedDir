#ifndef FIXED_POINT_TYPE_H
#define FIXED_POINT_TYPE_H
#include "ap_fixed.h"
#include "hls_math.h"
#define WORD_LENGTH 32
#define INT_LEN 11
	typedef  ap_fixed<WORD_LENGTH,INT_LEN,AP_TRN_ZERO,AP_SAT>  fixed_type;
#endif
