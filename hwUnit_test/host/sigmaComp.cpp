#include "sigmaComp.h"

extern "C"
{
void sigmaComp(fixed_type Pxx[NUM_VAR],
				fp_str& sigMat,
				fixed_type rnd_data[NUM_PARTICLES*NUM_VAR])
{
#pragma HLS PIPELINE off
	// this function computes the sigma values for the extrapolation
	fixed_type Pxx_local[NUM_VAR];
	fixed_type rnd_local[NUM_PARTICLES*NUM_VAR];
	fixed_type sigMat_local[NUM_VAR*NUM_PARTICLES];

	for(int i=0; i < NUM_VAR;i++)
	{
#pragma HLS UNROLL
		Pxx_local[i] = Pxx[i];
	}
	for(int i=0; i < NUM_PARTICLES*NUM_VAR;i++)
	{
#pragma HLS PIPELINE
		rnd_local[i] = rnd_data[i];
	}

	for (int i = 0; i < NUM_VAR; i++){
		sigmaComp_label0:for(int j=0; j < NUM_PARTICLES;j++){
			int k=i*NUM_PARTICLES+j;
#pragma HLS PIPELINE II=1
			// Matlab formula normrnd = random*sigma + mu
			// random is taken from rnd_data, sigma is taken =  sqrt(pxx(i,i)), and mu =0
			sigMat_local[i*NUM_PARTICLES+j] = rnd_local[k] * Pxx_local[i] + 0;
		}
	}
	store_sigma(sigMat_local,sigMat);
}
}
//#pragma HLS INTERFACE mode=m_axi bundle=gmem1 port=msmtinfo num_write_outstanding=1 max_write_burst_length=2 offset=slave
//#pragma HLS INTERFACE mode=m_axi bundle=gmem0 port=prtcls num_write_outstanding=1 max_write_burst_length=2 offset=slave

void store_sigma(
				fixed_type sigMat_local[NUM_VAR*NUM_PARTICLES],
				fp_str& sigMat)
{
	for(int i=0; i < NUM_VAR*NUM_PARTICLES;i++)
	{
#pragma HLS PIPELINE
		sigMat.write(sigMat_local[i]);
	}
}
