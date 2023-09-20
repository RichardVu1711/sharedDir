#include "Calweights.h"
//#include "read_write_csv.h"

#define LOOP_FACTOR 32
extern "C"{
void CalPzxZdiff(Mat* prtcls, msmt* msmtinfo1,msmt* msmtinfo2,msmt* msmtinfo3,
				int index,
				Mat_S* Pxx_,
				fixed_type zDiff[NUM_PARTICLES*N_MEAS],
				fixed_type pzx[NUM_PARTICLES*N_MEAS*N_MEAS])
{
#pragma HLS INTERFACE mode=m_axi bundle=gmem3 port=msmtinfo3 offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem2 port=msmtinfo2 offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem1 port=msmtinfo1 offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem0 port=prtcls offset=slave

#pragma HLS STABLE variable=index
#pragma HLS INTERFACE mode=s_axilite bundle=control port=prtcls
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=return
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=msmtinfo1
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=msmtinfo2
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=msmtinfo3
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=prtcls
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=index
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=Pxx_
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=zDiff
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=pzx




//	init_mat(&temp_X,NUM_VAR,1);
//	init_mat(&temp_X2,SN_NUM*2,NUM_VAR);
//	init_mat(&temp_X3,SN_NUM*2,1);
	fixed_type sum = 0.0;
	Mat_S R = R_cal(msmtinfo1);
	showmat_S(&R);
	likelihood:for(int j =0; j < NUM_PARTICLES;j++)
	{
		// reuse resources
		// tempX2 is Hxx, tempX3 is z_cap
#pragma HLS DATAFLOW
		Mat_S temp_X;
		msmt msmtinfo_2;
		Mat_S temp_X2;
		Mat_S temp_X3;
		Mat_S temp_X4;
		fixed_type pzx_fp[SN_NUM*2*SN_NUM*2];
		fixed_type zDiff_fp[SN_NUM*2];
		int idx1 = index;
		int idx2 = index;
		int nobs = msmtinfo1->n_aoa+msmtinfo1->n_tdoa;
		int j1 = j;
		int j2 = j;
		// duplicate data for further single-producer-consumer violation
		duplicate_data(prtcls,j1,&temp_X,&temp_X4);

//		Calculate the estimated measurement based on the given particles
		ObsJacobian(&temp_X,idx1,msmtinfo1,&temp_X2);
		GISobs_model(&temp_X4,idx2,msmtinfo2,&temp_X3);
//		cout << "Hxx:\n";
//		showmat_S(&temp_X2);
//		cout << "z_Cap\n";
//		showmat_S(&temp_X3);
		// calculate pzx and zDiff

//		GISPzx(msmtinfo3,Pxx_,&temp_X2,&temp_X3,pzx_fp,zDiff_fp);
//		GISPzx(msmtinfo3,Pxx_,&temp_X2,&temp_X3,&R,pzx_fp,zDiff_fp);

//
//		for(int i =0; i < SN_NUM*2;i++)
//		{
//			zDiff[j*SN_NUM*2+i] = zDiff_fp[i];
//			for(int k =0; k < SN_NUM*2;k++)
//			{
//				pzx[j*SN_NUM*2*SN_NUM*2+ i*SN_NUM*2+ k] = pzx_fp[i][k];
//			}
//		}
//		copy_result(pzx_fp,zDiff_fp,j2,zDiff,pzx);
//		cout << "z_diff:\n";
//		for(int i =0; i < SN_NUM*2;i++)
//		{
//			cout << zDiff_fp[i] << " ,";
//		}
//		cout <<"\n\n";
//		cout << "Hx:\n";
//		for(int i =0; i < SN_NUM*2;i++)
//		{
////			zDiff[j*SN_NUM*2+i] = zDiff_fp[i];
//			for(int k =0; k < SN_NUM*2;k++)
//			{
//				cout << pzx[j*SN_NUM*2*SN_NUM*2+ i*SN_NUM*2+ k] << ",";
//			}
//			cout <<"\n";
//		}
//		cout <<"\n";


	}
//	write_csv("C:/ESP_PF_PLNewWeight/PSPL_newPzxCal/TestData/result/pzx_newCal.csv",
//			convert_double(pzx,1,NUM_PARTICLES*N_MEAS*N_MEAS,-1),NUM_PARTICLES*N_MEAS,N_MEAS);
//	write_csv("C:/ESP_PF_PLNewWeight/PSPL_newPzxCal/TestData/result/zDiff_newCal.csv",
//			convert_double(zDiff,1,NUM_PARTICLES*N_MEAS,-1),NUM_PARTICLES,N_MEAS);

	// normalized weights
}
}


extern "C"
{
// unroll mean calculation
void mean_X_and_Pxx(Mat* prtcls, Mat_S* Pxx_)
{
#pragma HLS PIPELINE off
	Mat_S X_avg;
	X_avg.row = NUM_VAR;
	X_avg.col = 1;

	Pxx_->row = NUM_VAR;
	Pxx_->col = NUM_VAR;

//	show_mat(prtcls);
	Init_X:for(int i =0; i < NUM_VAR;i++)

#pragma HLS PIPELINE
X_avg.entries[i*NUM_VAR] = 0;
	// X_avg[i<2] = Sum(X[i<2]/1024)
	state_avg0:for(int i0 =0; i0 < 2*NUM_PARTICLES;i0+=NUM_PARTICLES)
	{
		fixed_type sum = 0;
		fixed_type sum_0 = 0;
		state_avg02:for(int i1 =0; i1 < NUM_PARTICLES; i1+=LOOP_FACTOR)
		{
			sum_0 = sum;
			state_avg03_add:for(int i2 =0; i2 < LOOP_FACTOR; i2++)
			{
#pragma HLS PIPELINE II=2
#pragma HLS UNROLL factor=4
						fixed_type temp = sum_0;
						fixed_type temp1 = prtcls->entries[i0+i1+i2]/(NUM_PARTICLES);
						sum_0 = temp +  temp1;
//						cout << sum_0 <<"\t";
			}
			sum = sum_0;
		}
		int step = i0/NUM_PARTICLES*NUM_VAR;
		X_avg.entries[step] = sum;
	}

	// X_avg[i>=2] = Sum(X[i>=2])/1024
	state_avg1:for(int i0 =2*NUM_PARTICLES; i0 < NUM_VAR*NUM_PARTICLES;i0+=NUM_PARTICLES)
	{
		fixed_type sum = 0;
		fixed_type sum_0 = 0;
		state_avg12:for(int i1 =0; i1 < NUM_PARTICLES; i1+=LOOP_FACTOR)
		{
			sum_0 = sum;
			state_avg13_add:for(int i2 =0; i2 < LOOP_FACTOR; i2++)
			{
#pragma HLS PIPELINE II=2
#pragma HLS UNROLL factor=4
						fixed_type temp = sum_0;
						sum_0 = temp +  prtcls->entries[i0+i1+i2] ;
			}
			sum = sum_0;
		}
		int step = i0/NUM_PARTICLES*NUM_VAR;
		X_avg.entries[step] = sum/NUM_PARTICLES;
	}
//	cout << "X_avg:\n";
//	showmat_S(&X_avg);
	fixed_type current_pxx[NUM_VAR*NUM_VAR];
	fixed_type temp_pxx[NUM_VAR*NUM_VAR];
	fixed_type temp_pxx2[NUM_VAR*NUM_VAR];

	fixed_type acc[NUM_VAR*(NUM_VAR+1)/2];
	pxx_calAvg(prtcls,&X_avg,acc);
//	showmat_S(&X_avg);
	int k =1;
//copy non-diagonal element
	for(int i =0; i < NUM_VAR;i++)
	{
		for(int j=i+1; j < NUM_VAR;j++)
		{
			Pxx_->entries[hash_map(i,j,1)] = acc[k];
			Pxx_->entries[hash_map(j,i,1)] = acc[k];
			k = k+1;
		}
		k=k+1;
	}
//	showmat_S(Pxx_);
// copy diagonal element
	k = 0;
	for(int i=0; i < NUM_VAR;i++)
	{
		Pxx_->entries[hash_map(i,i,1)] = acc[k];
//		cout << acc[k] << " at " << k  << " is written into " << hash_map(i,i,1) << "\n";
		k =k + 13-(i);
	}
//	showmat_S(Pxx_);
//	k =0;
//	for(int i=0; i < NUM_VAR;i++)
//	{
//		for(int j= i; j < NUM_VAR-i;j++)
//		{
//			cout << acc[k] << " ,";
//			k = k +1;
//		}
//		cout << "\n";
//	}
//	for(int i=0; i < 91; i++)
//		cout << acc[i] << " ,";
}
}
void pxx_calAvg(Mat* particle, Mat_S* state, fixed_type acc [NUM_VAR*(NUM_VAR+1)/2])
{
	fixed_type pxx_arr[NUM_VAR*(NUM_VAR+1)/2];
	fixed_type mul[NUM_VAR*(NUM_VAR+1)/2];

	// we will need to apply a reset to this kernel everytime we start.
	fixed_type sub[NUM_VAR];
	// length of arithmetic progression
	int acc_len = NUM_VAR*(NUM_VAR+1)/2;

	// first, we need to initialise the accumulator.
	memset(acc,0,(size_t) acc_len*sizeof(fixed_type));
//	cout << "particles\n";
//	show_mat(particle);
	// pxx = sum ((X-Xi)(X-Xi)'wi)
	// sub_result = X-Xi = m
	// mul = {sub0^2,sub1^2,sub^2,..,sub12^2,sub0sub1,sub0sub2,..,sub1sub2,sub1sub3,...,sub11sub12}
	// acc = acc + mul*wi.
	pxx_cal:for(int i =0; i < NUM_PARTICLES;i++)
	{
//#pragma HLS DATAFLOW
		// sub = X-Xi
		avg_selPrtcls(particle->entries,i,state,sub);
		// mul = {sub0^2,sub1^2,sub^2,..,sub12^2,sub0sub1,sub0sub2,..,sub1sub2,sub1sub3,...,sub11sub12}
		avg_mul(sub,sub,mul);
		// acc = acc + mul*wi.
		avg_acc(mul,acc);
	}
//	cout << "\n";
//	for(int i =0; i < NUM_VAR;i++)
//	{
//		cout << sub[i] << ", ";
//	}
//	cout << "\n";
//	for(int i =0; i < 91;i++)
//	{
//		cout << mul[i] << ", ";
//	}
//	cout << mul[13] << "-" << acc[13]<< ", ";
}
void avg_selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, fixed_type sub[NUM_VAR])
{
	for(int k=0; k < NUM_VAR;k++)
	{
		sub[k] = prtcls[k*NUM_PARTICLES+col] - X_avg->entries[k*NUM_VAR];
	}
}

// the multiplication of a vector and its transpose.
// mul = sub*sub'
// Instead of multiplying all elements in the vector, I did for a half only.
void avg_mul(fixed_type sub1[],fixed_type sub2[], fixed_type mul[])
{
	int mul_len = NUM_VAR*(NUM_VAR+1)/2;
	int k =0;
	mul_1:for(int i =0; i < NUM_VAR;i++)
	{
		mul_0:for(int j=i; j < NUM_VAR;j++)
		{
			mul[k++] = sub1[i]*sub2[j];

		}
	}
}

// accumulate mul*w.
void avg_acc(fixed_type mul[], fixed_type acc[])
{
	int acc_len = NUM_VAR*(NUM_VAR+1)/2;
	fixed_type over_Ns  = 1.0/NUM_PARTICLES;
	for(int i=0; i < acc_len;i++)
	{
		fixed_type temp = mul[i]*over_Ns;
		acc[i] = acc[i] + temp;
	}
}

// this function is used to copy a particle's vector into two vector.
// Those two generated vectors are used to calculate the estimated meas.
// This function is designed particularly for Dataflow, avoiding single-producer-consumer violations
void duplicate_data(Mat* in, int sel, Mat_S* out1, Mat_S* out2)
{
#pragma HLS inline off

	for(int i0 = 0; i0 < NUM_VAR;i0++)
	{
#pragma HLS pipeline
		out1->entries[i0*NUM_VAR] = in->entries[i0*NUM_PARTICLES+sel];
		out2->entries[i0*NUM_VAR] = in->entries[i0*NUM_PARTICLES+sel];
	}
	out1->col = 1;
	out1->row = NUM_VAR;
	out2->col = 1;
	out2->row = NUM_VAR;
}
void copy_result(	fixed_type pzx_fp[SN_NUM*2][SN_NUM*2],
					fixed_type zDiff_fp[SN_NUM*2],
					int j,
					fixed_type zDiff[NUM_PARTICLES*SN_NUM*2],
					fixed_type pzx[NUM_PARTICLES*SN_NUM*2*SN_NUM*2])
{
#pragma HLS inline off
	for(int i =0; i < SN_NUM*2;i++)
	{
		zDiff[j*SN_NUM*2+i] = zDiff_fp[i];
		for(int k =0; k < SN_NUM*2;k++)
		{
			pzx[j*SN_NUM*2*SN_NUM*2+ i*SN_NUM*2+ k] = pzx_fp[i][k];
		}
	}
}
