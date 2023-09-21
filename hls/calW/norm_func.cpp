//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: norm_func.cpp
//
// MATLAB Coder version            : 5.2
// C/C++ source code generated on  : 26-Jan-2022 13:02:19
//

// Include Files
#include "../calW/norm_func.h"

#include <cmath>

// Function Definitions
//
// Arguments    : const float X[4]
// Return Type  : float
//
void norm_func(fixed_type X[2],fixed_type* norm_error)
{
	ap_fixed<WORD_LENGTH+INT_LEN,INT_LEN+INT_LEN> a = X[0]*X[0] + X[1]*X[1];
	*norm_error = hls::sqrt(a);
}
// File trailer for norm_func.cpp
//
// [EOF]
//
