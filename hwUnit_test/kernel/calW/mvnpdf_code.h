//
// Academic License - for use in teaching, academic research, and meeting
// course requirements at degree granting institutions only.  Not for
// government, commercial, or other organizational use.
// File: mvnpdf_code.h
//
// MATLAB Coder version            : 5.2
// C/C++ source code generated on  : 02-Feb-2022 00:16:46
//

#ifndef MVNPDF_CODE_H
#define MVNPDF_CODE_H

// Include Files
#include "tmwtypes.h"
#include <cstddef>
#include <cstdlib>
#include "../lib/mat_lib.h"
#include "../lib/global_define.h"

// Function Declarations
extern fixed_type mvnpdf_code(fixed_type zCap_in[6], fixed_type Mu[6],
							fixed_type Pzx[6*6], int obs);
extern "C"{
void mvnpdf_fpCall(fixed_type zCap_in[N_MEAS],
					fixed_type Pzx[N_MEAS*N_MEAS],
					int n_obs, fixed_type p_cal[1]);
}
#endif
//
// File trailer for mvnpdf_code.h
//
// [EOF]
//
