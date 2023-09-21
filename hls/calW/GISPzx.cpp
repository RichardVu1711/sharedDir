#include "../calW/GISPzx.h"

void zDiff_cal(Mat_S* z_cap,
			msmt* msmtinfo,
			fixed_type zDiff_local[NUM_PARTICLES*N_MEAS],
			int n_obs, int idx)
{
	zDiff_loop:for(int i=0; i < N_MEAS;i++)
	{
		if(i < n_obs){
			zDiff_local[idx*N_MEAS+ i] = z_cap->entries[i*NUM_VAR] - msmtinfo->z.entries[i*NUM_VAR];
		}
		else{
			zDiff_local[idx*N_MEAS+ i] = 1023;
		}

	}
}
