#include "GISPzx.h"
extern "C"{
void GISPzx(msmt* msmtinfo, Mat_S* Pxx, Mat_S* Hxx, Mat_S* z_cap,
			fixed_type pzx_fp[N_MEAS*N_MEAS],fixed_type zDiff_fp[N_MEAS])
{
#pragma HLS PIPELINE off
	fixed_type p =0;
	Mat_S R = R_cal(msmtinfo);
	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;

	// temp_Pzx = Hx*Pxx
	Mat_S temp_Pzx;
	init_mat(&temp_Pzx,n_aoa+n_tdoa,NUM_VAR);
	multiply_S(Hxx,Pxx,&temp_Pzx);
//	printf("Pzx = Hx*Pxx");
//	showmat_S(&temp_Pzx);
//	printf("Hx");
//	showmat_S(Hxx);
//	printf("Pxx");
//	showmat_S(Pxx);
	Mat_S T_Hxx;
	init_mat(&T_Hxx,NUM_VAR,n_aoa+n_tdoa);
	CTranspose_S(Hxx,&T_Hxx);
//	printf("Hxx'");
//	showmat_S(&T_Hxx);
	// Pzx = temp_Pzx*Hx' = Hx*PxxHx'
	Mat_S Pzx;
	init_mat(&Pzx,n_aoa+n_tdoa,n_aoa+n_tdoa);
	multiply_S(&temp_Pzx,&T_Hxx,&Pzx);
//	printf("Pzx = Hx*Pxx*Hx'");
//	showmat_S(&Pzx);


	// Addition: Pzx = Pzx + R
	sum_S(&Pzx,&R,&Pzx);
//	printf("Pzx = temp_Pzx+R = Hx*PxxHx' + R");
//	showmat_S(&Pzx);

	//T_Hxx = Pzx'
	init_mat(&T_Hxx,n_aoa+n_tdoa,n_aoa+n_tdoa);
	CTranspose_S(&Pzx,&T_Hxx);
//	printf("T_Hxx = Pzx'");
//	showmat_S(&T_Hxx);
	pzx_cal:for(int i =0; i <n_aoa+n_tdoa;i++) //row
	{
		for(int j=0; j < n_aoa+n_tdoa;j++) //col
		{
			fixed_type re = (get_ele_S(&Pzx,i,j)+get_ele_S(&T_Hxx,i,j))/2;
//			set_ele_S(&Pzx,i,j,re);
			pzx_fp[hash_map(i,j,3)] = re;
		}
	}

	zDiff_cal:for(int i =0; i <n_aoa+n_tdoa;i++) //row
	{
			fixed_type re = get_ele_S(&(msmtinfo->z),i,0)-get_ele_S(z_cap,i,0);
			zDiff_fp[i] = re;
	}

//	cout << "\n previous solution: \n";
//	for(int i=0; i < N_MEAS;i++)
//	{
//		for(int j=0; j < N_MEAS;j++)
//		{
//			cout << pzx_fp[hash_map(i,j,3)] << ", ";
//		}
//		cout << "\n";
//	}
//	cout << "\n";

}
}
Mat_S R_cal(msmt* msmtinfo)
{
#pragma HLS PIPELINE off
	fixed_type AOAstd_sqr = AOASTD_SQRT;
	fixed_type TDOAstd_sqr = TDOASTD_SQRT;
	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;
	Mat_S R_noise;
	init_mat(&R_noise,n_aoa+n_tdoa,n_aoa+n_tdoa);

	for(int i =0; i < n_aoa+n_tdoa;i++)
	{
		for(int j =0; j <  n_aoa+n_tdoa;j++)
		{
			set_ele_S(&R_noise,i,j,0);
		}
		if(i < n_aoa)
			set_ele_S(&R_noise,i,i,AOAstd_sqr);
		else
			set_ele_S(&R_noise,i,i,TDOAstd_sqr);
	}
	return R_noise;
}
