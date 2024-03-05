#include "ObsJacobian.h"

void ObsJacobian(fixed_type prtcl_X[NUM_VAR],
				int step, msmt* msmtinfo, fixed_type H[N_MEAS][NUM_VAR])
{
#pragma HLS PIPELINE off
//	cout << "X est\n";
//	showmat_S(prtcl_X);

	Mat_S H_temp;
	H_temp.row = N_MEAS;
	H_temp.col = NUM_VAR;
	fixed_type SNx[SN_NUM*2] = {0,0,-1.9561,-739.8094,-383.0758,-2.3068};

	fixed_type x = prtcl_X[0];
	fixed_type y = prtcl_X[1];

	fixed_type diffX[SN_NUM];
	fixed_type diffY[SN_NUM];

	fixed_type devx [SN_NUM] ={1,1,1};

//	cout << "ObsJacobian\n";
	prepare_var:for(int i = 0; i < SN_NUM;i++)
	{
		diffX[i] = SNx[i*2] - x;
		diffY[i] = SNx[i*2+1] - y;

		if(diffX[i] > 30 || diffY[i] > 30 || diffX[i] < -30 || diffY[i] < -30) devx[i] =128;
	}

	// calculate est AOA ObsJacobian
	// General, est AOA =  (x or y)/(x^2 + y^2)
	// H(i,:) = [y/(x^2 + y^2) -x/(x^2 + y^2) 0 0 zeros(1,6) 1 0 0];

	aoaCal:for(int i =0; i< SN_NUM;i++)
	{
		static fixed_type H_row1[NUM_VAR] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
		static fixed_type H_row2[NUM_VAR] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
		static fixed_type H_row3[NUM_VAR] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
		fixed_type temp_x,temp_y,temp_num,temp_dev,temp_h0,temp_h1;
		// currently, I used truncating method, 128 = 8 bits throwing away.
		temp_x = diffX[i]/devx[i]; // x/dev
		temp_y = diffY[i]/devx[i]; // y/dev
		temp_dev = devx[i];
		switch(i+1)
		{
		case 1: //SN1
			setRowMat_S(&H_temp,i,H_row1);
			break;
		case 2: // SN2
			setRowMat_S(&H_temp,i,H_row2);
			break;
		case 3: //SN3
			setRowMat_S(&H_temp,i,H_row3);
			break;
		}
		// temp_x^2 + temp_y^2
		temp_num = temp_x*temp_x+ temp_y*temp_y;
		temp_h0 = temp_y/temp_dev/temp_num;
		temp_h1 = -temp_x/temp_dev/temp_num;
		set_ele_S(&H_temp,i,0,temp_h0);
		set_ele_S(&H_temp,i,1,temp_h1);
	}

	// these values are decleared outside the loop is to save some values and computation times
	// since we don't need to calculate step+2 every iteration. It is a constant and we want declear once only
	// 	step =1 since init state =  1 => this is est of meas. cov => step  = current step +1
	fixed_type re[2],re2[2];
	fixed_type r1, r2;
	fixed_type H_row1[NUM_VAR] = {0, 0, 0, 0, 1, 0, 0, step+2, 0, 0, 0, 0, 0};
	fixed_type H_row2[NUM_VAR] = {0, 0, 0, 0, 0, 1, 0, 0, step+2, 0, 0, 0, 0};
	fixed_type H_row3[NUM_VAR] = {0, 0, 0, 0, 0, 0, 1, 0, 0, step+2, 0, 0, 0};
	fixed_type delx, dely,SNx_X,SNx_Y,SNy_X,SNy_Y;
	int i1x,i1y,i2x,i2y;
	// ObsJacobian TDOA calculate
	tdoaTag:for(int i =0; i< SN_NUM;i++)
	{


		switch(i + 1)
		{
			case 1: //12
				// choosing data index
				setRowMat_S(&H_temp,i+SN_NUM,H_row1);
				i1x = 0;
				i1y = 1;
				i2x = 2;
				i2y = 3;
				break;
			case 2: //13
				setRowMat_S(&H_temp,i+SN_NUM,H_row2);
				i1x = 0;
				i1y = 1;
				i2x = 4;
				i2y = 5;
				break;
			case 3: //23
				setRowMat_S(&H_temp,i+SN_NUM,H_row3);
				i1x = 2;
				i1y = 3;
				i2x = 4;
				i2y = 5;
				break;
		}
		// First SN
		re[0] = prtcl_X[0] - SNx[i1x];
		re[1] = prtcl_X[1] - SNx[i1y];
		// Second SN
		re2[0] = prtcl_X[0] - SNx[i2x];
		re2[1] = prtcl_X[1] - SNx[i2y];
//		cout << "re = " << re[0] << ", " <<  re[1] << "\n";
//		cout << "re2 = " << re2[0] << ", " <<  re2[1] << "\n";

// 		data preparation for further computation
		SNx_X = SNx[i1x];
		SNx_Y = SNx[i1y];

		SNy_X = SNx[i2x];
		SNy_Y = SNx[i2y];
//		calculate the norm distance = sqrt(r1[0]^2 + r1[1]^2)
		norm_func(re,&r1);
//		cout << r1 << ",\t" << r2 << "\n";
		norm_func(re2,&r2);
//		cout << r2 << ",\t" << r1 << "\n";
//		cout << r1 << ",\t" << r2 << "\n";

		delx = del(x,SNx_X,SNy_X,r1,r2);
		dely = del(y,SNx_Y,SNy_Y,r1,r2);
//		cout << delx << "\t" << dely << "\n";
		set_ele_S(&H_temp,i+SN_NUM,0,delx);
		set_ele_S(&H_temp,i+SN_NUM,1,dely);
	}
//	cout << "before re-aligned\n";


	// check valid TDOA and AOA measurement.
	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;

	int outputIdx =0;
	for(int i=0; i < N_MEAS;i++){
		if(msmtinfo->validIdx[i])
		{
			for(int j=0; j < NUM_VAR;j++){
				H[outputIdx][j] = H_temp.entries[i*NUM_VAR +j];
			}
			outputIdx++;
		}
	}
	if(outputIdx !=  (n_aoa + n_tdoa)){
		printf("The number of Valid data is not match !!\n");
		//make it failed fast
		for(int i=0;  i< N_MEAS;i++){
			if(i >= n_aoa+n_tdoa){
				for(int j=0; j < NUM_VAR;j++){
					H[i][j] = 1023;
				}
			}
		}

	}
//	showmat_S(H);
}

fixed_type del(fixed_type var, fixed_type s1, fixed_type s2, fixed_type r1, fixed_type r2)
{
#pragma HLS INLINE off
	fixed_type fs_c = FS_C;
	return fs_c*((var-s1)/r1 - (var-s2)/r2);

}

