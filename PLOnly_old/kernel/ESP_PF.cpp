#include "ESP_PF.h"


// this is the big programs of ESP_PF
// 0 means no error
// 1 means an error that I don't know where it comes from
// I should have an error table in the future implementation
extern "C"
{
void ESP_PF_Wrapper(Mat_S* obs_FP,Mat_S* pxx_FP,Mat_S* state_FP, fixed_type wt[NUM_PARTICLES], Mat* prtcls_in,
					Mat_S* stateOut, Mat_S* pxxOut, Mat* prtcl_out, fixed_type wtOut[NUM_PARTICLES],
					int step)
{
#pragma HLS PIPELINE off

//	int i=0;
//	// initialize if step ==0;


	ESP_PF(obs_FP,pxx_FP,state_FP,wt,prtcls_in,step);

//assign input pointers with outputs.
	stateOut->row = NUM_VAR;
	stateOut->col = 1;
	for(int i =0; i<NUM_VAR;i++)
	{
		stateOut->entries[i*NUM_VAR] = state_FP->entries[i*NUM_VAR];
	}

	for(int i =0; i<NUM_VAR*NUM_VAR;i++)
	{
		pxxOut->entries[i] = pxx_FP->entries[i];
	}
	pxxOut->row = NUM_VAR;
	pxxOut->col = NUM_VAR;
	for(int i =0; i<NUM_PARTICLES*NUM_VAR;i++)
	{
		prtcl_out->entries[i] = prtcls_in->entries[i];
	}
	prtcl_out->row = NUM_VAR;
	prtcl_out->col = NUM_PARTICLES;
	for(int i =0; i < NUM_PARTICLES;i++)
	{
		wtOut[i] = wt[i];
	}
}
}
void ESP_PF(Mat_S* obs_FP,Mat_S* Pxx_FP,Mat_S* state_FP, fixed_type wt[NUM_PARTICLES], Mat* prtcls_act, int step)
{
	#pragma HLS PIPELINE off

	int dt = 1;
	// SAMPLING
	fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES];
	fixed_type rnd_rk4[NUM_VAR*4];
	fixed_type rnd_rsmp;
//	auto start = high_resolution_clock::now();
	// generate random numbers for sigma computation
//	cout << "HellowWorld";
	for(int i=0; i< NUM_VAR*NUM_PARTICLES;i++)
	{
		double rnd_temp;
		randn(&rnd_temp,0,0);
		rnd_sigma[i] = rnd_temp;
	}

	// generate random numbers for rk4
//		cout << "random\n";
	for(int i=0; i< NUM_VAR*4;i++)
	{
		double rnd_temp;
		randn(&rnd_temp,0,0);
		rnd_rk4[i] = rnd_temp;
	}
	ESPCrtParticles(state_FP,Pxx_FP,prtcls_act,rnd_sigma,rnd_rk4);

//	 CALCULATE WEIGHTS
	Calweights(wt,prtcls_act,obs_FP,step);
//
	//	CALCULATE N_eff
	double N_eff=0;
	double re_sum =0;
	for(int j =0; j < NUM_PARTICLES;j++){
		double temp2 = wt[j];
		re_sum += (temp2*temp2);
	}

	N_eff = 1/re_sum;
	printf("N_eff: %f\n",N_eff);
	if(N_eff < NUM_PARTICLES*0.5 || N_eff > NUM_PARTICLES*4 )
	{
		double r;
		randn(&r,0,0);
		fixed_type r_fp = r;
		resamplePF_wrap(prtcls_act,wt,r_fp);

	}

	PFupdate(prtcls_act,wt,Pxx_FP,state_FP);
//	cout << state_FP->entries[0] << ", " << state_FP->entries[13] << ", " << N_eff;
}
