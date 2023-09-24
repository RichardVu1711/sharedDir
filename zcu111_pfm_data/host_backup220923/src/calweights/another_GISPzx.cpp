#include "another_GISPzx.h"
extern "C"{
void another_GISPzx(msmt* msmtinfo, Mat_S* Pxx, Mat_S* Hxx, Mat_S* z_cap, Mat_S* R_mat,
			fixed_type pzx_fp[N_MEAS*N_MEAS],fixed_type zDiff_fp[N_MEAS])
{
#pragma HLS PIPELINE off
// let calculate diagonal elements in pzx first
// for diagonal elements constant, the combination
// calculate as row_x'*row_x


	// diagonal elements => using symmetric
	fixed_type pzx_temp[6][6];
	fixed_type Hxx_comb[PXX_AP_LEN];
	for(int i=0; i < N_MEAS;i++)
	{
		for(int j=0; j < N_MEAS;j++)
		{
			pzx_temp[i][j] = 0;
		}
	}
	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;

	for(int i=0; i < N_MEAS;i++)
	{
		symmetric_mul(Hxx, i, Hxx_comb);
		elementwise_acc(Hxx_comb,Pxx,get_ele_S(R_mat,i,i),&(pzx_temp[i][i]));
		pzx_fp[hash_map(i,i,3)] = pzx_temp[i][i];
	}


	// non-diagonal elements => using asymmetric
	for(int i=0; i < N_MEAS;i++)
	{
		for(int j=i+1; j < N_MEAS;j++)
		{
			asymmetric_mul(Hxx, i,j, Hxx_comb);
			elementwise_acc(Hxx_comb,Pxx,get_ele_S(R_mat,i,j),&(pzx_temp[i][j]));
			pzx_fp[hash_map(j,i,3)] = pzx_temp[i][j];
		    pzx_fp[hash_map(i,j,3)] = pzx_temp[i][j];
		}
	}


	for(int i=0; i < N_MEAS;i++)
		zDiff_fp[i] = get_ele_S(&(msmtinfo->z),i,0)-get_ele_S(z_cap,i,0);
}
}

void asymmetric_mul(Mat_S* Hxx, int n_row, int s_row,fixed_type mult_rst[PXX_AP_LEN])
{
	int k=0;
//	int s_row = n_row+1;
	for(int i=0; i < NUM_VAR;i++)
	{
		for(int j=i; j < NUM_VAR;j++)
		{
			if(i!=j)
			{
				fixed_type temp0= Hxx->entries[hash_map(n_row,i,1)]*Hxx->entries[hash_map(s_row,j,1)];
				fixed_type temp1= Hxx->entries[hash_map(n_row,j,1)]*Hxx->entries[hash_map(s_row,i,1)];
				mult_rst[k] = temp0+temp1;
			}
			else
			{	// diagonal multiplication
				mult_rst[k] = Hxx->entries[hash_map(n_row,i,1)]*Hxx->entries[hash_map(s_row,i,1)];
			}
			k = k+1;
		}
	}
}


// this is the trick multiplication
// where the transpose of a row is multiply with its self.
// hence, instead of calculate n*n mult, we need to do n*(n-1)/2 (n=6 for now)
void symmetric_mul(Mat_S* Hxx, int n_row, fixed_type mult_rst[PXX_AP_LEN])
{
	int k=0;
	for(int i=0; i < NUM_VAR;i++)
	{
		for(int j=i; j < NUM_VAR;j++)
		{
			fixed_type temp = Hxx->entries[hash_map(n_row,i,1)]*Hxx->entries[hash_map(n_row,j,1)];

			if(i!=j) mult_rst[k] = temp *2;
			else mult_rst[k] = temp;
			k = k+1;
		}
	}

//	cout << "Combinations:\n";
//	k=0;
//	for(int i=0; i < NUM_VAR;i++)
//	{
//		for(int j=i; j < NUM_VAR;j++)
//		{
//			cout << mult_rst[k++] << "\t";
//		}
//		cout << "\n";
//	}
//	cout << "\n";
//	for(int i=0; i < NUM_VAR;i++)
//		cout <<Hxx->entries[hash_map(n_row,i,1)] << ", ";
//	cout << "\n";
}

void elementwise_acc(fixed_type Hxx_combination[PXX_AP_LEN],Mat_S* pxx, fixed_type R_ele, fixed_type* acc)
{
	int k =0;
	acc[0]=0;

	for(int i=0; i < NUM_VAR;i++)
	{
		for (int j=i; j < NUM_VAR;j++)
		{
			// need an improvement, to transfer Hxx_combination index to pxx index
			// for example i = 0:90;
			// i = 13 => pxx[14];
			acc[0] = acc[0] + Hxx_combination[k]*pxx->entries[hash_map(i,j,1)];
			k = k+1;
		}
	}
	acc[0] = acc[0] + R_ele;

//	cout << "\n" << acc[0] << "\n";
}

//Mat_S R_cal(msmt* msmtinfo)
//{
//	fixed_type AOAstd_sqr = AOASTD_SQRT;
//	fixed_type TDOAstd_sqr = TDOASTD_SQRT;
//	int n_aoa = msmtinfo->n_aoa;
//	int n_tdoa = msmtinfo->n_tdoa;
//	Mat_S R_noise;
//	init_mat(&R_noise,n_aoa+n_tdoa,n_aoa+n_tdoa);
//
//	for(int i =0; i < n_aoa+n_tdoa;i++)
//	{
//		for(int j =0; j <  n_aoa+n_tdoa;j++)
//		{
//			set_ele_S(&R_noise,i,j,0);
//		}
//		if(i < n_aoa)
//			set_ele_S(&R_noise,i,i,AOAstd_sqr);
//		else
//			set_ele_S(&R_noise,i,i,TDOAstd_sqr);
//	}
//	return R_noise;
//}
