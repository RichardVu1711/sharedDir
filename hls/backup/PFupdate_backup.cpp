#include "PFupdate.h"
#define LOOP_FACTOR 32
void pxx_cal(Mat* particle, Mat_S* state,fixed_type wt[], fixed_type acc [NUM_VAR*(NUM_VAR+1)/2])
{

	// length of arithmetic progression
	int acc_len1 = NUM_VAR*(NUM_VAR+1)/2;
	// first, we need to initialise the accumulator.
	memset(acc,0,(size_t) acc_len1*sizeof(fixed_type));

	// pxx = sum ((X-Xi)(X-Xi)'wi)
	// sub_result = X-Xi = m
	// mul = {sub0^2,sub1^2,sub^2,..,sub12^2,sub0sub1,sub0sub2,..,sub1sub2,sub1sub3,...,sub11sub12}
	// acc = acc + mul*wi.
	pxx_cal:for(int i =0; i < NUM_PARTICLES;i++)
	{
//#pragma HLS DATAFLOW
		int acc_len = NUM_VAR*(NUM_VAR+1)/2;

		fixed_type pxx_arr[NUM_VAR*(NUM_VAR+1)/2];
		fixed_type mul[NUM_VAR*(NUM_VAR+1)/2];

		// we will need to apply a reset to this kernel everytime we start.
	//	static fixed_type acc[NUM_VAR*(NUM_VAR+1)/2] = {0};
		fixed_type sub[NUM_VAR];
		// sub = X-Xi
		Up_selPrtcls(particle->entries,i,state,sub);
		// mul = {sub0^2,sub1^2,sub^2,..,sub12^2,sub0sub1,sub0sub2,..,sub1sub2,sub1sub3,...,sub11sub12}
		Up_mul(sub,sub,mul);
		// acc = acc + mul*wi.
		Up_acc(mul,wt[i],acc);
	}
}
void Up_selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, fixed_type sub[NUM_VAR])
{
	for(int k=0; k < NUM_VAR;k++)
	{
		sub[k] = prtcls[k*NUM_PARTICLES+col] - X_avg->entries[k*NUM_VAR];
	}
}

// the multiplication of a vector and its transpose.
// mul = sub*sub'
// Instead of multiplying all elements in the vector, I did for a half only.
void Up_mul(fixed_type sub1[],fixed_type sub2[], fixed_type mul[])
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
void Up_acc(fixed_type mul[], fixed_type wt, fixed_type acc[])
{
	int acc_len = NUM_VAR*(NUM_VAR+1)/2;
	for(int i=0; i < acc_len;i++)
	{
		fixed_type temp = mul[i]*wt;
		acc[i] = acc[i] + temp;
	}
}

void load_PFU_data(fixed_type prtcls[NUM_VAR*NUM_PARTICLES],
					fixed_type wt[NUM_PARTICLES],
					Mat* prtcls_local,
					fixed_type wt_local[NUM_PARTICLES])
{
	for(int i=0; i < NUM_VAR*NUM_PARTICLES;i++)
	{
		prtcls_local->entries[i] = prtcls[i];
	}
	for(int i=0; i < NUM_PARTICLES;i++)
	{
		wt_local[i] = wt[i];
	}
}

void store_PFU_data(Mat_S* state_local, Mat_S* pxx_local,
					fixed_type state[NUM_VAR],
					fixed_type pxx[NUM_VAR*NUM_VAR])
{
	for(int i=0; i < NUM_VAR;i++)
	{
		state[i] = state_local->entries[i*NUM_VAR];
	}
	for(int i=0; i < NUM_VAR*NUM_VAR;i++)
	{
		pxx[i] = pxx_local->entries[i];
	}
}


extern "C"{
void PFupdate(fixed_type particle[NUM_VAR*NUM_PARTICLES],
			fixed_type wt[NUM_PARTICLES],
			fixed_type state[NUM_VAR],
			fixed_type pxx[NUM_VAR*NUM_VAR])
{
#pragma HLS INTERFACE mode=m_axi bundle=gmem0 port=particle offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem1 port=state offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem2 port=pxx offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem3 port=wt

#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=return
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=particle
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=wt
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=state
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=pxx

#pragma HLS PIPELINE off

	Mat particles_local;
	particles_local.col = NUM_PARTICLES;
	particles_local.row = NUM_VAR;

	Mat_S state_local;
	state_local.row = NUM_VAR;
	state_local.col =1;

	Mat_S pxx_local;
	pxx_local.row = NUM_VAR;
	pxx_local.col = NUM_VAR;

	fixed_type wt_local[NUM_PARTICLES];
	load_PFU_data(particle,wt,&particles_local,wt_local);
	Init_X:for(int i =0; i < NUM_VAR;i++)
	{
#pragma HLS PIPELINE
		state_local.entries[i*NUM_VAR] = 0;
	}
	Init_Pxx:for(int i=0; i <NUM_VAR*NUM_VAR; i++)
	{
		pxx_local.entries[i] = 0;
	}
//	memset(&pxx->entries,0,NUM_VAR*NUM_VAR)
	// calculate the mean estimation
	state_mean1:for(int i0 =0*NUM_PARTICLES; i0 < NUM_VAR*NUM_PARTICLES;i0+=NUM_PARTICLES)
	{
		fixed_type sum = 0;
		fixed_type sum_0 = 0;
		state_mean12:for(int i1 =0; i1 < NUM_PARTICLES; i1+=LOOP_FACTOR)
		{
			sum_0 = sum;
			state_mean13_add:for(int i2 =0; i2 < LOOP_FACTOR; i2++)
			{
#pragma HLS PIPELINE II=2
#pragma HLS UNROLL factor=4
				fixed_type temp = sum_0;
				// shift the weight to the right 8 bits
				fixed_type temp_wt = wt_local[i1+i2]*128;
				fixed_type temp1 = particles_local.entries[i0+i1+i2]*wt_local[i1+i2];
				sum_0 = sum_0 +  temp1;
			}
			sum = sum_0;
		}
		int step = i0/NUM_PARTICLES*NUM_VAR;
		state_local.entries[step] = sum;
	}

// The sum of arithemic progression with step of 1 is  n(n+1)/2
//
//	// first, we need to initialise the accumulator.
//	memset(acc,0,(size_t) acc_len);
//
//	// pxx = sum ((X-Xi)(X-Xi)'wi)
//	// sub_result = X-Xi = m
//	// mul = {sub0^2,sub1^2,sub^2,..,sub12^2,sub0sub1,sub0sub2,..,sub1sub2,sub1sub3,...,sub11sub12}
//	// acc = acc + mul*wi.

	fixed_type acc[NUM_VAR*(NUM_VAR+1)/2];
	pxx_cal(&particles_local,&state_local,wt_local,acc);
	int k =1;
//copy non-diagonal element
	for(int i =0; i < NUM_VAR;i++)
	{
		for(int j=i+1; j < NUM_VAR;j++)
		{
			pxx_local.entries[hash_map(i,j,1)] = acc[k];
			pxx_local.entries[hash_map(j,i,1)] = acc[k];
			k = k+1;
		}
		k=k+1;
	}
// copy diagonal element
	k = 0;
	for(int i=0; i < NUM_VAR;i++)
	{
		pxx_local.entries[hash_map(i,i,1)] = acc[k];
		k =k + 13-(i);
	}
	store_PFU_data(&state_local,&pxx_local,state,pxx);
}
}

