#include "ObsJacobian.h"
;
Mat_S ObsJacobian(Mat_S* prtcl_X, int step, msmt* msmtinfo)
{
//	showmat_S(prtcl_X);
	fixed_type SNx[6] = {0,0,-1.9561,-739.8094,-383.0758,-2.3068};

	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;
	int aoaTag[3];
	int tdoaTag[3];
	Mat_S H;
	init_mat(&H,n_aoa+n_tdoa,NUM_VAR);

	fixed_type x = get_ele_S(prtcl_X,0,0);
	fixed_type y = get_ele_S(prtcl_X,1,0);

	fixed_type diffX[SN_NUM];
	fixed_type diffY[SN_NUM];

	fixed_type devx [SN_NUM] ={1,1,1};
//	cout << "ObsJacobian\n";
	prepare_var:for(int i = 0; i < SN_NUM;i++)
	{
		aoaTag[i]= msmtinfo->aoaIdx[i];
		tdoaTag[i] = msmtinfo->tdoaIdx[i];
		diffX[i] = SNx[i*2] - x;
		diffY[i] = SNx[i*2+1] - y;

		if(diffX[i] > 30 || diffY[i] > 30 || diffX[i] < -30 || diffY[i] < -30) devx[i] =128;
	}

	// calculate est AOA ObsJacobian
	// General, est AOA =  (x or y)/(x^2 + y^2)
	// H(i,:) = [y/(x^2 + y^2) -x/(x^2 + y^2) 0 0 zeros(1,6) 1 0 0];

	int k =0;

	aoaCal:for(int i =0; i< SN_NUM;i++)
	{
		fixed_type H_row1[NUM_VAR] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
		fixed_type H_row2[NUM_VAR] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
		fixed_type H_row3[NUM_VAR] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
		fixed_type temp_x,temp_y,temp_num,temp_dev,temp_h0,temp_h1;

		switch(aoaTag[i])
		{
		case 1: //SN1
			// currently, I used truncating method, 128 = 8 bits throwing away.
			temp_x = diffX[0]/devx[0]; // x/dev
			temp_y = diffY[0]/devx[0]; // y/dev
			temp_dev = devx[0];
			// temp_x^2 + temp_y^2
			setRowMat_S(&H,k++,H_row1);
			break;
		case 2: // SN2
			temp_x = diffX[1]/devx[1]; // x/dev
			temp_y = diffY[1]/devx[1]; // y/dev
			temp_dev = devx[1];
			setRowMat_S(&H,k++,H_row2);
			break;
		case 3: //SN3
			temp_x = diffX[2]/devx[2]; // x/dev
			temp_y = diffY[2]/devx[2]; // y/dev
			temp_dev = devx[2];
			// temp_x^2 + temp_y^2
			setRowMat_S(&H,k++,H_row3);
			break;
		}

		// temp_x^2 + temp_y^2
		temp_num = temp_x*temp_x+ temp_y*temp_y;
		temp_h0 = temp_y/temp_dev/temp_num;
		temp_h1 = -temp_x/temp_dev/temp_num;
		set_ele_S(&H,k-1,0,temp_h0);
		set_ele_S(&H,k-1,1,temp_h1);


	}


	k = n_aoa;

	fixed_type re[2],re2[2];
	fixed_type r1, r2;
	fixed_type H_row1[NUM_VAR] = {0, 0, 0, 0, 1, 0, 0, step+2, 0, 0, 0, 0, 0};
	fixed_type H_row2[NUM_VAR] = {0, 0, 0, 0, 0, 1, 0, 0, step+2, 0, 0, 0, 0};
	fixed_type H_row3[NUM_VAR] = {0, 0, 0, 0, 0, 0, 1, 0, 0, step+2, 0, 0, 0};


	// ObsJacobian TDOA calculate
	tdoaTag:for(int i =0; i< SN_NUM;i++)
	{
	//  xf::solver::gesvj  return
	//	|a11 a21 a12 a22|
	//	|a11 a12|
	//	|a21 a22|
		fixed_type delx, dely,SNx_X,SNx_Y,SNy_X,SNy_Y;
	// 	step =1 since init state =  1 => this is est of meas. cov => step  = current step +1


		switch(tdoaTag[i])
		{
		//norm(Matrix A 2x2) =  max(V) with V is SVD of A
			case 1: //12
				//SN1
				re[0] = {get_ele_S(prtcl_X,0,0) - SNx[0]};
				re[1] = {get_ele_S(prtcl_X,1,0) - SNx[1]};
				//SN2
				re2[0] = {get_ele_S(prtcl_X,0,0) - SNx[2]};
				re2[1] = {get_ele_S(prtcl_X,1,0) - SNx[3]};
//				temp = norm_func(re);
//				r2 = temp;
				SNx_X = SNx[0];
				SNy_X = SNx[2];
				SNx_Y = SNx[1];
				SNy_Y = SNx[3];

				setRowMat_S(&H,k++,H_row1);
				break;
			case 2: //13
				//SN1
				re[0] = {get_ele_S(prtcl_X,0,0) - SNx[0]};
				re[1] = {get_ele_S(prtcl_X,1,0) - SNx[1]};
				//SN3
				re2[0] = {get_ele_S(prtcl_X,0,0) - SNx[4]};
				re2[1] = {get_ele_S(prtcl_X,1,0) - SNx[5]};
//				temp = norm_func(re);
//				r3 = temp;

				SNx_X = SNx[0];
				SNy_X = SNx[4];
				SNx_Y = SNx[1];
				SNy_Y = SNx[5];
				setRowMat_S(&H,k++,H_row2);
				break;
			case 3: //23
				//SN2
				re[0] = {get_ele_S(prtcl_X,0,0) - SNx[2]};
				re[1] = {get_ele_S(prtcl_X,1,0) - SNx[3]};
				//SN3
				re2[0] = {get_ele_S(prtcl_X,0,0) - SNx[4]};
				re2[1] = {get_ele_S(prtcl_X,1,0) - SNx[5]};
//				temp = norm_func(re);
//				r3 = temp;
				SNx_X = SNx[2];
				SNy_X = SNx[4];
				SNx_Y = SNx[3];
				SNy_Y = SNx[5];

				setRowMat_S(&H,k++,H_row3);
				break;
		}
		norm_func(re,&r1);
//		cout << r1 << ",\t" << r2 << "\n";
		norm_func(re2,&r2);
//		cout << r2 << ",\t" << r2 << "\n";
//		cout << r1 << ",\t" << r2 << "\n";

		delx = del(x,SNx_X,SNy_X,r1,r2);
		dely = del(y,SNy_Y,SNy_Y,r1,r2);
//		cout << delx << "\t" << dely << "\n";
		set_ele_S(&H,k-1,0,delx);
		set_ele_S(&H,k-1,1,dely);
	}
//	cout << "\n\n";
//	showmat_S(&H);
	return H;
}

fixed_type del(fixed_type var, fixed_type s1, fixed_type s2, fixed_type r1, fixed_type r2)
{
#pragma HLS INLINE
	fixed_type fs_c = FS_C;
	return fs_c*((var-s1)/r1 - (var-s2)/r2);

}

