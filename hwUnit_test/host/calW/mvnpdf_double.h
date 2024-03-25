//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: mvnpdf_code.h
//
// MATLAB Coder version            : 5.2
// C/C++ source code generated on  : 02-Feb-2022 00:16:46
//

#pragma once

// Include Files
#include "tmwtypes.h"
#include <cstddef>
#include <cstdlib>
#include "../lib/global_define.h"
#include "../lib/mat_lib.h"

// Function Declarations
extern "C" {
	extern double mvnpdf_double(double zCap_in[N_MEAS],
							double Pzx[N_MEAS*N_MEAS], int n_obs);
}
//
// File trailer for mvnpdf_code.h
//
// [EOF]
//
