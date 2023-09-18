#include "GISPzx.h"
#include <cstdint>
#include <cstring>

// all diagonal and non-diagonal index in 2d coordinate
#define BIT_LEN 8

const ap_uint<BIT_LEN> index_map0x[PXX_AP_LEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6, 7,
													8, 9, 10, 11, 12, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 3, 4, 5, 6,
													7, 8, 9, 10, 11, 12, 4, 5, 6, 7, 8, 9, 10, 11, 12, 5, 6, 7, 8, 9,
													10, 11, 12, 6, 7, 8, 9, 10, 11, 12, 7, 8, 9, 10, 11, 12, 8, 9, 10,
													11, 12, 9, 10, 11, 12, 10, 11, 12, 11, 12, 12};
const ap_uint<BIT_LEN> index_map0y[PXX_AP_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
													1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
													2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3,
													3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
													4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6,
													6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8,
													8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 11, 11, 12};
const ap_uint<BIT_LEN> index_asymY[N_ASYM] ={0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4 };
const ap_uint<BIT_LEN> index_asymX[N_ASYM] ={1, 2, 3, 4, 5, 2, 3, 4, 5, 3, 4, 5, 4, 5, 5 };
const ap_uint<BIT_LEN> index_sym[N_SYM] = {0,1,2,3,4,5};

const ap_uint<BIT_LEN>  index_map2[N_MEAS*(N_MEAS+1)/2][2] = {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {1, 1},
																{1, 2}, {1, 3}, {1, 4}, {1, 5}, {2, 2}, {2, 3}, {2, 4},
																{2, 5}, {3, 3}, {3, 4}, {3, 5}, {4, 4}, {4, 5}, {5, 5}};


void pzx_cal(fixed_type Hxx_local[NUM_VAR*NUM_VAR], fixed_type pxx_local[NUM_VAR*NUM_VAR], Mat_S* R_mat,
			fixed_type pzx_fp[N_MEAS][N_MEAS])
{
// #pragma HLS DATAFLOW
//	fixed_type comb_asym[PXX_AP_LEN*N_ASYM];
//	fixed_type comb_sym[PXX_AP_LEN*N_SYM];
//
//	fixed_type Hxx_local1[N_MEAS*NUM_VAR];
//	fixed_type Hxx_local2[N_MEAS*NUM_VAR];
//
//	fixed_type pxx_local1[NUM_VAR*NUM_VAR];
//	fixed_type pxx_local2[NUM_VAR*NUM_VAR];
//
//	fixed_type acc_sym[N_SYM];
//	fixed_type acc_asym[N_ASYM];
//
//	duplicate_pzxDF(Hxx_local,pxx_local,Hxx_local1,Hxx_local2,pxx_local1,pxx_local2);
//
//	symmetric_mul(Hxx_local1, comb_sym);
//	asymmetric_mul(Hxx_local2, comb_asym);
//
//	elewise_symAcc(comb_sym,pxx_local,R_mat,acc_sym);
//	elewise_asymAcc(comb_asym,pxx_local,acc_asym);
//
////	cp_pzx(acc_sym,acc_asym,pzx_fp);
}
void cp_pzx(fixed_type acc_sym[N_SYM], fixed_type acc_asym[N_ASYM], fixed_type pzx[N_MEAS][N_MEAS], msmt* msmtinfo, Mat_S* z_cap, fixed_type zDiff[N_MEAS] )
{

	for (int i=0; i <N_ASYM;i++)
	{
#pragma HLS PIPELINE off

		int idx1 = index_asymY[i];
		int idx2 = index_asymX[i];
		pzx[idx1][idx2] = acc_asym[i];
		pzx[idx2][idx1] = acc_asym[i];
	}
	for(int i=0; i < N_SYM;i++)
	{
#pragma HLS PIPELINE II=1

		pzx[i][i] = acc_sym[i];
	}
	for(int i=0; i < N_MEAS;i++)
	{
#pragma HLS PIPELINE II=1
		zDiff[i] = msmtinfo->z.entries[i*13] - z_cap->entries[i*13];
	}
}
void duplicate_pzxDF(fixed_type  hxx_local[NUM_VAR*NUM_VAR],fixed_type  pxx_local[NUM_VAR*NUM_VAR],
				fixed_type Hxx_local1[N_MEAS*NUM_VAR],fixed_type Hxx_local2[N_MEAS*NUM_VAR],
				fixed_type pxx_local1[NUM_VAR*NUM_VAR], fixed_type pxx_local2[NUM_VAR*NUM_VAR])
{
	memcpy(Hxx_local1, hxx_local, (size_t) N_MEAS*NUM_VAR*sizeof(fixed_type));
	memcpy(Hxx_local2, hxx_local, (size_t) N_MEAS*NUM_VAR*sizeof(fixed_type));
	memcpy(pxx_local1, pxx_local, (size_t) NUM_VAR*NUM_VAR*sizeof(fixed_type));
	memcpy(pxx_local2, pxx_local, (size_t) NUM_VAR*NUM_VAR*sizeof(fixed_type));
}
void idx_extract( int i, int* idx1, int* idx2)
{
#pragma HLS inline off
	idx1[0] = index_map2[i][0];	//i
	idx2[0] = index_map2[i][1];	//j
}

void asymmetric_mul(fixed_type Hxx[], fixed_type mult_rst[PXX_AP_LEN*N_ASYM])
{

#pragma HLS ALLOCATION function instances=asymmetric_mul limit=1
	fixed_type r1[NUM_VAR], r2[NUM_VAR];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=r1
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=r2


	//extract rows from Hxx matrix
	big_asym:for(int m=0; m < N_ASYM*PXX_AP_LEN;m = m+ PXX_AP_LEN)
	{
		int n_row,s_row;
		int current =m/PXX_AP_LEN;
		n_row = index_asymY[current];
		s_row = index_asymX[current];
		memcpy(r1,&(Hxx[n_row*NUM_VAR]),(size_t) NUM_VAR*sizeof(fixed_type));
		memcpy(r2,&(Hxx[s_row*NUM_VAR]),(size_t) NUM_VAR*sizeof(fixed_type));
		asym_mul:for(int i=0; i < PXX_AP_LEN;i++)
		{
	#pragma HLS PIPELINE II=1
			int idx1 = index_map0y[i];	//i
			int idx2 = index_map0x[i];	//j
			fixed_type temp0, temp1;
	#pragma HLS BIND_OP variable=temp0 op=mul impl=fabric latency=1
	#pragma HLS BIND_OP variable=temp1 op=mul impl=dsp latency=1

			if(idx1 != idx2)
			{
				temp0= r1[idx1]*r2[idx2];
				temp1= r1[idx2]*r2[idx1];

			}
			else
			{
				temp0 = r1[idx1]*r2[idx2];
				temp1 =0;
			}
			mult_rst[i+m] = temp0+temp1;
		}
	}

}


// this is the trick multiplication
// where the transpose of a row is multiply with its self.
// hence, instead of calculate n*n mult, we need to do n*(n-1)/2 (n=6 for now)
void symmetric_mul(fixed_type Hxx[],  fixed_type mult_rst[PXX_AP_LEN*N_SYM])
{
#pragma HLS ALLOCATION function instances=symmetric_mul limit=1
	fixed_type r[NUM_VAR];
//#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=r
#pragma HLS BIND_STORAGE variable=r type=ram_2p impl=bram

	big_sym:for(int m=0; m < N_SYM*PXX_AP_LEN;m=m+PXX_AP_LEN)
	{
		int current = m/PXX_AP_LEN;
		int n_row = index_sym[current];
		fixed_type temp = 0;
#pragma HLS BIND_OP variable=temp op=mul impl=fabric latency=1
		memcpy(r,&(Hxx[n_row*NUM_VAR]),(size_t) NUM_VAR*sizeof(fixed_type));

		sym_mul:for(int i=0; i < PXX_AP_LEN;i++)
		{
#pragma HLS PIPELINE
			int idx1 = index_map0y[i];	//i
			int idx2 = index_map0x[i];	//j
			temp = r[idx1]*r[idx2];
			if(idx1 != idx2) temp = temp *2;
			else  temp = temp;
			mult_rst[i+m] = temp;
		}
	}

}

void elewise_asymAcc(fixed_type Hxx_combination[PXX_AP_LEN*N_ASYM],fixed_type pxx[NUM_VAR*NUM_VAR], fixed_type acc[N_ASYM])
{



// **** NOTE this pipeline only work when NUM_VAR is an odd number !!!

	int current =0;
	big_acc: for(int m=0; m < PXX_AP_LEN*N_ASYM; m= m+PXX_AP_LEN)
	{
		fixed_type acc_[NUM_VAR];
		fixed_type mul_[PXX_AP_LEN/NUM_VAR];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=acc_
	#pragma HLS BIND_OP variable=mul_ op=mul impl=fabric latency=1
		memset(acc_,0,(size_t) (NUM_VAR)*sizeof(fixed_type));
		acc[current] = 0;
		arr_loop:for(int i=0; i < PXX_AP_LEN;i=i+PXX_AP_LEN/NUM_VAR)
		{
	// PXX_AP_LEN/NUM_VAR = (NUM_VAR - 1)*1/2
			int pos = i/(PXX_AP_LEN/NUM_VAR);
		#pragma HLS PIPELINE II = 7
			arr_loop0:for(int j=0; j < PXX_AP_LEN/NUM_VAR;j++)
			{
				int idx1 = index_map0y[i+j];	//i
				int idx2 = index_map0x[i+j];	//j
				mul_[j] = Hxx_combination[i+j+m]*pxx[hash_map(idx1,idx2,1)];
				acc_[pos] += mul_[j];
			}
		}
		sum_loop:for(int i=0;  i < NUM_VAR; i++ )
		{
		#pragma HLS UNROLL
			acc[current] += acc_[i];
		}
		current++;
	}
}
void elewise_symAcc(fixed_type Hxx_combination[PXX_AP_LEN*N_SYM],fixed_type pxx[NUM_VAR*NUM_VAR], Mat_S* R_ele, fixed_type acc[N_SYM])
{




	int current =0;
	fixed_type R_dia [N_SYM];
	for(int i=0; i < N_SYM;i++)
		R_dia[i] = R_ele->entries[i*NUM_VAR+i];
	big_acc: for(int m=0; m < PXX_AP_LEN*N_SYM; m= m+PXX_AP_LEN)
	{
		fixed_type acc_[NUM_VAR];
		fixed_type mul_[PXX_AP_LEN/NUM_VAR];

	//#pragma HLS BIND_OP variable=acc_ op=add impl=dsp latency=1
	#pragma HLS BIND_OP variable=mul_ op=mul impl=dsp latency=1

	// **** NOTE this pipeline only work when NUM_VAR is an odd number !!!
	#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=acc_
		memset(acc_,0,(size_t) (NUM_VAR)*sizeof(fixed_type));
		acc[current] = 0;

		arr_loop:for(int i=0; i < PXX_AP_LEN;i=i+PXX_AP_LEN/NUM_VAR)
		{
	// PXX_AP_LEN/NUM_VAR = (NUM_VAR - 1)*1/2
			int pos = i/(PXX_AP_LEN/NUM_VAR);
		#pragma HLS PIPELINE II = 7
			arr_loop0:for(int j=0; j < PXX_AP_LEN/NUM_VAR;j++)
			{
				int idx1 = index_map0y[i+j];	//i
				int idx2 = index_map0x[i+j];	//j
				mul_[j] = Hxx_combination[i+j+m]*pxx[hash_map(idx1,idx2,1)];
				acc_[pos] += mul_[j];
			}
		}
		sum_loop:for(int i=0;  i < NUM_VAR; i++ )
		{
		#pragma HLS UNROLL
			acc[current] += acc_[i];
		}
		acc[current] = acc[current] + R_dia[current];
		current++;
	}
}
Mat_S R_cal(int n_aoa, int n_tdoa)
{
	fixed_type AOAstd_sqr = AOASTD_SQRT;
	fixed_type TDOAstd_sqr = TDOASTD_SQRT;
//	int n_aoa = n_aoa;
//	int n_tdoa = n_tdoa;
	Mat_S R_noise;
	init_mat(&R_noise,n_aoa+n_tdoa,n_aoa+n_tdoa);

	n_meas1:for(int i =0; i < n_aoa+n_tdoa;i++)
	{
#pragma HLS LATENCY max=36 min=1
#pragma HLS LOOP_TRIPCOUNT avg=6 max=6 min=1
		n_meas0:for(int j =0; j <  n_aoa+n_tdoa;j++)
		{
#pragma HLS LATENCY max=6 min=1
#pragma HLS LOOP_TRIPCOUNT avg=6 max=6 min=1
			set_ele_S(&R_noise,i,j,0);
		}
		if(i < n_aoa)
			set_ele_S(&R_noise,i,i,AOAstd_sqr);
		else
			set_ele_S(&R_noise,i,i,TDOAstd_sqr);
	}
	return R_noise;
}
