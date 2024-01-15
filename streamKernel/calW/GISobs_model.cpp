#include "GISobs_model.h"

void GISobs_model(fixed_type prtcl_X[NUM_VAR],
				int step, msmt* msmtinfo, fixed_type zCap[N_MEAS])
{
#pragma HLS PIPELINE off
	fixed_type SNx[6] = {0,0,-1.9561,-739.8094,-383.0758,-2.3068};
	// SNx is the location of the sensor node in x-y coordinator
	// SN1_x SN1_y SN2_x SN2_y SN3_x SN3_y

//	Mat_S z_temp;
//	z_temp.row = N_MEAS;
//	z_temp.col = 1;
	fixed_type z_temp[N_MEAS];

	fixed_type x = prtcl_X[0];
	fixed_type y = prtcl_X[1];

	fixed_type diffX[SN_NUM]={0,0,0};
	fixed_type diffY[SN_NUM]={0,0,0};

	fixed_type devx [SN_NUM] ={1,1,1};

	for(int i = 0; i < SN_NUM;i++)
	{
		diffX[i] = x - SNx[i*2];
		diffY[i] = y - SNx[i*2+1];

		// check if the X and Y distance is too big
		// if they are greater than 30, needs to scale them down before calculating the square root
		// or square of these values
		if(diffX[i] > 30 || diffY[i] > 30 || diffX[i] < -30 || diffY[i] < -30) devx[i] =128;
	}
	// In this method: we calculate all of 6 SNs first, then we re-organised the particles order
	// based on
	// calculate AOA est
	for(int i =0; i < SN_NUM;i++)
	{
		if(msmtinfo->validIdx[i]){
			ap_fixed<WORD_LENGTH,INT_LEN> a = diffY[i];
			ap_fixed<WORD_LENGTH,INT_LEN> b = diffX[i];
	//		cout << a <<", " << b << "\n";
			fixed_type temp = hls::atan2(b,a);

			if(temp < 0) temp = temp +(fixed_type) 6.283185; //2pi

			temp += prtcl_X[10+i];
			z_temp[i] = temp;
		}
	}
	// calculate TDOA est

	// 1023 means invalid
	// 3 is maximum values for n_row or n_col
	// aoaIdx contains what node is triggered SN1 =01, SN2= 02, SN3 =03
	// tdoaIdx contains what node is triggered 12 = 01, 13= 02, 23 = 03
	for(int i =0; i < N_TDOA;i++)
	{
		// only compute if valid
		if(msmtinfo->validIdx[i+N_AOA]){
			fixed_type temp_sqrt1,temp_sqrt2;
			int i1,i2;
			switch(i+1)
			{
				case 1: // 1 is 12
					i1 = 0;
					i2 = 1;
					break;
				case 2: // 2 is 13
					i1 = 0;
					i2 = 2;
					break;
				case 3: // 3 is 23
					i1 = 1;
					i2 = 2;
					break;
			}
			ap_fixed<WORD_LENGTH,INT_LEN> a= (diffX[i1]/devx[i1])*(diffX[i1]/devx[i1]) + (diffY[i1]/devx[i1])*(diffY[i1]/devx[i1]);

			ap_fixed<WORD_LENGTH,INT_LEN> b= (diffX[i2]/devx[i2])*(diffX[i2]/devx[i2]) + (diffY[i2]/devx[i2])*(diffY[i2]/devx[i2]);
			temp_sqrt1 = hls::sqrt(a);
			temp_sqrt2 = hls::sqrt(b);

			fixed_type fs_c =FS_C;
			fixed_type temp_re = fs_c*(temp_sqrt1*devx[i1] - temp_sqrt2*devx[i2])
						+prtcl_X[4+i] +prtcl_X[7+i]*(step+2);
			// continue after TDOA calculation
			z_temp[N_AOA+i] = temp_re;
		}
	}

	// check valid TDOA and AOA measurement.
	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;

	int outputIdx =0;
	for(int i=0; i < N_MEAS;i++){
		if(msmtinfo->validIdx[i])
		{
			zCap[outputIdx] = z_temp[i];
			outputIdx++;

		}
	}
	if(outputIdx !=  (n_aoa + n_tdoa)){
		printf("The number of Valid data is not match !!\n");
		//make it failed fast
		for(int i=0;  i< N_MEAS;i++)
		{
			if(i >= n_aoa+n_tdoa){
				zCap[i] = 1023;
			}
		}

	}
//	showmat_S(z_cap);

}
