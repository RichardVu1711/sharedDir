#include "GISPzx.h"
#include "mvnpdf_code.h"
fixed_type GISPzx(msmt* msmtinfo, Mat_S* Pxx, Mat_S* Hxx, Mat_S* z_cap)
{
#pragma HLS INTERFACE mode=s_axilite port=return register storage_impl=bram
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


	Mat_S T_Hxx;
	init_mat(&T_Hxx,NUM_VAR,n_aoa+n_tdoa);
	CTranspose_S(Hxx,&T_Hxx);

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

	double pzx_du[6][6];
	pzx_cal:for(int i =0; i <n_aoa+n_tdoa;i++) //row
	{
		for(int j=0; j < n_aoa+n_tdoa;j++) //col
		{
//			fixed_type pzx_ = get_ele_S(Pzx,i,j);
//			fixed_type pzx_T = get_ele_S(T_Hxx,i,j);
			fixed_type re = (get_ele_S(&Pzx,i,j)+get_ele_S(&T_Hxx,i,j))/2;
			set_ele_S(&Pzx,i,j,re);
			pzx_du[i][j] = re;
		}
	}

//	printf("Pzx = 1/2(Pzx+Pzx')");
//	showmat_S(&Pzx);

	Mat_S zDiff;
	init_mat(&zDiff,n_aoa+n_tdoa,1);
	double zDiff_du[6];
	zDiff_cal:for(int i =0; i <n_aoa+n_tdoa;i++) //row
	{
//			fixed_type pzx_ = get_ele_S(Pzx,i,j);
//			fixed_type pzx_T = get_ele_S(T_Hxx,i,j);
			fixed_type re = get_ele_S(&(msmtinfo->z),i,0)-get_ele_S(z_cap,i,0);
			set_ele_S(&zDiff,i,0,re);
			zDiff_du[i] = re;
	}
	double Mu[6]= {0,0,0,0,0,0};
//	printf("zDiff = msmtinfo.z - z_cap");
//	showmat_S(&zDiff);

//	printf("pzx");
//	showmat_S(&Pzx);
	Mat_S Mu_fp;
	init_mat(&Mu_fp,6,1);
	for(int i =0; i <6;i++)
	{
		set_ele_S(&Mu_fp,i,0,0);
	}
	double p_du = mvnpdf_code(zDiff_du, Mu,pzx_du,Pzx.row);
//	fixed_type p_new = mvnpdf(&zDiff,&Mu_fp,&Pzx);

//	double temp = p_new;
//	printf("%f\t\n",temp);
	p = p_du;
	return p;
}

Mat_S R_cal(msmt* msmtinfo)
{
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
