#include "mean_pxx.h"
#define LOOP_FACTOR 32

void load_mean_data(fp_str& prtcls,
					Mat* prtcls_local)
{

	for(int i=0; i < NUM_VAR*NUM_PARTICLES;i++)
	{
#pragma HLS PIPELINE
		prtcls_local->entries[i] =prtcls.read();
	}
}

void store_mean_data(Mat_S* Pxx_local,
					fixed_type Pxx_[NUM_VAR*NUM_VAR])
{
	for(int i=0; i < NUM_VAR*NUM_VAR;i++)
	{
#pragma HLS PIPELINE
		Pxx_[i] = Pxx_local->entries[i];
	}
}


extern "C"
{
// unroll mean calculation
void mean_Pxx(fp_str& prtcls,
			 fixed_type Pxx_[NUM_VAR*NUM_VAR])
{
#pragma HLS PIPELINE off
	Mat_S X_avg;
	X_avg.row = NUM_VAR;
	X_avg.col = 1;

	Mat prtcls_local;
	prtcls_local.col = NUM_PARTICLES;
	prtcls_local.row = NUM_VAR;

	Mat_S Pxx_local;
	Pxx_local.row = NUM_VAR;
	Pxx_local.col = NUM_VAR;

	load_mean_data(prtcls,&prtcls_local);

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
						fixed_type temp1 = prtcls_local.entries[i0+i1+i2]/(NUM_PARTICLES);
						sum_0 = temp +  temp1;
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
						sum_0 = temp +  prtcls_local.entries[i0+i1+i2] ;
			}
			sum = sum_0;
		}
		int step = i0/NUM_PARTICLES*NUM_VAR;
		X_avg.entries[step] = sum/NUM_PARTICLES;
	}
	fixed_type current_pxx[NUM_VAR*NUM_VAR];
	fixed_type temp_pxx[NUM_VAR*NUM_VAR];
	fixed_type temp_pxx2[NUM_VAR*NUM_VAR];

	fixed_type acc[NUM_VAR*(NUM_VAR+1)/2];
	pxx_calAvg(&prtcls_local,&X_avg,acc);
	int k =1;
//copy non-diagonal element
	for(int i =0; i < NUM_VAR;i++)
	{
		for(int j=i+1; j < NUM_VAR;j++)
		{
			Pxx_local.entries[hash_map(i,j,1)] = acc[k];
			Pxx_local.entries[hash_map(j,i,1)] = acc[k];
			k = k+1;
		}
		k=k+1;
	}
// copy diagonal element
	k = 0;
	for(int i=0; i < NUM_VAR;i++)
	{
		Pxx_local.entries[hash_map(i,i,1)] = acc[k];
		k =k + 13-(i);
	}
	store_mean_data(&Pxx_local,Pxx_);

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

