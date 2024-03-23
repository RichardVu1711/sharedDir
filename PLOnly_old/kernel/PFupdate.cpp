#include "PFupdate.h"
#define LOOP_FACTOR 32
void PFupdate(Mat* particle, fixed_type wt[NUM_PARTICLES], Mat_S* pxx,Mat_S* state)
{
#pragma HLS PIPELINE off

	state->row = NUM_VAR;
	state->col =1;

	pxx->row = NUM_VAR;
	pxx->col = NUM_VAR;


//	Init_X:for(int i =0; i < NUM_VAR;i++)
//	{
//#pragma HLS PIPELINE
//			state->entries[i*NUM_VAR] = 0;
//	}
//	Init_Pxx:for(int i=0; i <NUM_VAR*NUM_VAR; i++)
//	{
//		pxx->entries[i] = 0;
//	}

	size_t nvar_size = NUM_VAR*NUM_VAR*sizeof(fixed_type);
	memset(state, 0, nvar_size);
	memset(pxx, 0, nvar_size);
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
						fixed_type temp1 =  particle->entries[i0+i1+i2]*wt[i1+i2];
						sum_0 = temp + temp1;
			}
			sum = sum_0;
		}
		int step = i0/NUM_PARTICLES*NUM_VAR;
		state->entries[step] = sum;
	}
//	showmat_S(state);
	Mat_S temp_X2;
	init_mat(&temp_X2,NUM_VAR,1);
	// calculate the mean covariance
	fixed_type current_pxx[NUM_VAR*NUM_VAR];
	fixed_type temp_pxx[NUM_VAR*NUM_VAR];
	fixed_type temp_pxx2[NUM_VAR*NUM_VAR];
	for(int i=0; i < NUM_PARTICLES;i++)
	{
		// current_pxx = pxx_ => prepare for the accumulator
		Up_loadPxx(pxx->entries,current_pxx);
//		showmat_S(Pxx_);
		// temp_X2 = X_avg - X[i], where X[i] is the ith particle.
		// temp_X2 = X_avg - prtcls[i].
		// cout<< "X- X_avg:";
		// showmat_S(&temp_X2);
		Up_selPrtcls(particle->entries,i,state,&temp_X2);

		// temp_cal = (X-X_avg)*(X - X_avg)' = tempX2*tempX2'
		Up_mulColRow(temp_X2.entries,temp_pxx);

		// temp_cal = temp_cal/Ns before joining the sum to solve the limitation breakpoint problems.
		// Pxx_ = Pxx_ + ((X-X_)*(X-X_)')/Ns =  current_pxx + temp_pxx2/Ns
		Up_sumAcc(current_pxx,temp_pxx,wt[i],pxx->entries);
	}
}

void Up_selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, Mat_S* temp_X2)
{
	for(int k=0; k < NUM_VAR;k++)
	{
#pragma HLS UNROLL
		temp_X2->entries[k*NUM_VAR] = prtcls[k*NUM_PARTICLES+col] - X_avg->entries[k*NUM_VAR];
	}
	temp_X2->row= NUM_VAR;
	temp_X2->col = 1;
}
// this is a simple matrix multiplication of A * A', where A is 13x1 matrix
void Up_mulColRow(fixed_type inMat[], fixed_type outMat[])
{
	int k =0;
	// [13,1] x [1,13] = [13,13]
	// where outMat[i,j] = A[i,1]*A[j,1];
	matMul:for(int i =0; i < NUM_VAR;i++)
	{

		rowMul:for(int j =0; j < NUM_VAR;j++)
		{
#pragma HLS UNROLL
			outMat[k] = inMat[i*NUM_VAR]*inMat[j*NUM_VAR];
			k = k+1;
		}
	}
}
void Up_loadPxx(fixed_type inMat[], fixed_type outMat[])
{
	loadPxx_label2:for(int i=0; i < NUM_VAR*NUM_VAR;i++)
	{
#pragma HLS UNROLL
		outMat[i] = inMat[i];
	}
}

// Accumulating the Pxx sum. inMat
// Pxx_ = Pxx_ + ((X-X_)*(X-X_)')/Ns =  current_pxx + temp_pxx2/Ns
// currPxx: Pxx_
// newCov: (X-X_)*(X-X_)'
void Up_sumAcc(fixed_type currPxx[],fixed_type newCov[], fixed_type wt,fixed_type outMat[])
{
	// Pxx_ = Pxx_ + (X-X_)*(X-X_)' = temp_pxx2 + current_pxx
	sumAcc_label2:for(int i=0; i < NUM_VAR*NUM_VAR;i+=NUM_VAR)
		{
			sumAcc_label0:for(int i1=0; i1 < NUM_VAR;i1++)
			{
#pragma HLS UNROLL
				fixed_type temp = newCov[i+i1]*wt;
				outMat[i+i1] = currPxx[i+i1] + temp;
			}
		}
}
