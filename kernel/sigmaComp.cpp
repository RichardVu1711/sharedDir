#include "sigmaComp.h"

extern "C"
{
void sigmaComp(fixed_type Pxx[NUM_VAR*NUM_VAR],
				fixed_type sigMat[NUM_VAR*NUM_PARTICLES],
				fixed_type rnd_data[NUM_PARTICLES*NUM_VAR])
{
#pragma HLS INTERFACE mode=m_axi bundle=gmem2 port=rnd_data num_write_outstanding=1 max_write_burst_length=2 offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem1 port=sigMat num_read_outstanding=1 max_read_burst_length=2 offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem0 port=Pxx num_write_outstanding=1 max_write_burst_length=2 offset=slave

#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=return
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=rnd_data
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=sigMat
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=Pxx

#pragma HLS PIPELINE off
	// this function computes the sigma values for the extrapolation
//	newmat(sigMat,NUM_VAR,NUM_PARTICLES); // fixed 13xNUM_PARTICLES
	fixed_type Pxx_local[NUM_VAR*NUM_VAR];
	fixed_type rnd_local[NUM_PARTICLES*NUM_VAR];
	fixed_type sigMat_local[NUM_VAR*NUM_PARTICLES];

	for(int i=0; i < NUM_VAR*NUM_VAR;i++)
	{
#pragma HLS UNROLL
		Pxx_local[i] = Pxx[i];
	}
	for(int i=0; i < NUM_PARTICLES*NUM_VAR;i++)
	{
#pragma HLS UNROLL
		rnd_local[i] = rnd_data[i];
	}


	int k=0;
	for (int i = 0; i < NUM_VAR; i++)
	{
		ap_fixed<WORD_LENGTH,INT_LEN> temp = Pxx_local[i*NUM_VAR+i];
		fixed_type sqrtRe = hls::sqrt(temp);
		sigmaComp_label0:for(int j=0; j < NUM_PARTICLES;j++)
		{
#pragma HLS PIPELINE II=1
			// Matlab formula normrnd = random*sigma + mu
			// random is taken from rnd_data, sigma is taken =  sqrt(pxx(i,i)), and mu =0
			fixed_type r = rnd_local[k++] * sqrtRe + 0;
			sigMat_local[i*NUM_PARTICLES+j] = r;
		}
	}
	store_sigma(sigMat_local,sigMat);
}
}
//#pragma HLS INTERFACE mode=m_axi bundle=gmem1 port=msmtinfo num_write_outstanding=1 max_write_burst_length=2 offset=slave
//#pragma HLS INTERFACE mode=m_axi bundle=gmem0 port=prtcls num_write_outstanding=1 max_write_burst_length=2 offset=slave

void store_sigma(fixed_type sigMat_local[NUM_VAR*NUM_PARTICLES],
					fixed_type sigMat[NUM_VAR*NUM_PARTICLES])
{
	for(int i=0; i < NUM_VAR*NUM_PARTICLES;i++)
	{
#pragma HLS UNROLL
		sigMat[i] = sigMat_local[i];
	}
}
