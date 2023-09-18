#include "IS_dynamic.h"
#include "iostream"

void IS_dynamics(const fixed_type X[NUM_VAR], fixed_type dX[NUM_VAR], fixed_type rnd_data[NUM_VAR])
{

	fixed_type a[NUM_VAR*NUM_VAR] = {
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	      0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	fixed_type d;
	fixed_type d1;
	fixed_type r[NUM_VAR];
	Diff_cal:for (int i = 0; i < NUM_VAR; i++)
	{
		fixed_type r_fp = rnd_data[i];
		d = r_fp * sigma[i];
		r[i] = d;
		d1 = 0.0;
		for (int i1 = 0; i1 < NUM_VAR; i1++) {
		  d1 += a[i + NUM_VAR * i1] * X[i1];
		}
		dX[i] = d1 + d;
	}

}
