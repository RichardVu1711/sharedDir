#include "ESP_PF.h"


// this is the big programs of ESP_PF
// 0 means no error
// 1 means an error that I don't know where it comes from
// I should have an error table in the future implementation
extern "C"
{
void ESP_PF_Wrapper(Mat_S* obs,fixed_type pxx_in[NUM_VAR*NUM_VAR],fixed_type state_In[NUM_VAR], 
					fixed_type wt[NUM_PARTICLES],
					fixed_type pxxOut[NUM_VAR*NUM_VAR],
					fixed_type stateOut[NUM_VAR], 
					fixed_type wtOut[NUM_PARTICLES],
					int step, int seed)
{
#pragma HLS PIPELINE off

//	int i=0;
	ESP_PF(obs,pxx_in,state_In,wt,step, seed);

//assign input pointers with outputs.
	for(int i =0; i<NUM_VAR;i++)
	{
		stateOut[i] = state_In[i];
	}

	for(int i =0; i<NUM_VAR*NUM_VAR;i++)
	{
		pxxOut[i] = pxx_in[i];
	}
	for(int i =0; i < NUM_PARTICLES;i++)
	{
		wtOut[i] = wt[i];
	}
}
}

void ESP_PF(Mat_S* obs,fixed_type pxx[NUM_VAR*NUM_VAR],fixed_type state[NUM_VAR], 
			fixed_type wt[NUM_PARTICLES], 
			int step, int seed)
{
	#pragma HLS PIPELINE off

	int dt = 1;
	fixed_type prtcls[NUM_VAR*NUM_PARTICLES];
	fixed_type sigMat[NUM_VAR*NUM_PARTICLES];
	fixed_type Pxx_[NUM_VAR*NUM_VAR];

	// SAMPLING
	fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES];
	fixed_type rnd_rk4[NUM_VAR*4];
	fixed_type rnd_rsmp;
//	auto start = high_resolution_clock::now();
	// generate random numbers for sigma computation
//	cout << "HellowWorld";
	for(int i=0; i< NUM_VAR*NUM_PARTICLES;i++){
		double rnd_temp;
		if((step == 0)&&(i==0))
			rnd_temp =  RNG_withSeed(1,seed);
		else
			rnd_temp =  RNG_withSeed(0,seed);
		rnd_sigma[i] = rnd_temp;
	}

	// generate random numbers for rk4
	for(int i=0; i< NUM_VAR*4;i++){
		double rnd_temp;
		rnd_temp = RNG_withSeed(0,0);
		rnd_rk4[i] = rnd_temp;
	}

	
	// rk4 ------------------------------------//
	Mat_S stateOut_pro;
	Mat_S M_state;
	M_state.col = 1;
	M_state.row = NUM_VAR;
	for(int i=0; i < 13;i++)
	{
		M_state.entries[i*NUM_VAR] = state[i];
	}

	rk4(&M_state,&stateOut_pro,rnd_rk4);
	// sigmaComp ----------------------------//
	sigmaComp(pxx, sigMat,rnd_sigma);

	fixed_type state_pro[NUM_VAR];
	for(int i=0; i < NUM_VAR;i++)
	{
		state_pro[i] = stateOut_pro.entries[i*NUM_VAR];
	}

	// ESPCrtParticles ---------------------//
	ESPCrtParticles(state_pro,sigMat,prtcls);

//	 write_csv("/home/mylord/esp_pf/data/result/sigMat.csv",convert_double(sigMat,NUM_VAR*1024,0),NUM_VAR,1024);
//	 write_csv("/home/mylord/esp_pf/data/result/prtcls.csv",convert_double(prtcls,1,NUM_VAR*1024,0),NUM_VAR,1024);

	// mean_Pxx --------------------------//
	mean_Pxx(prtcls,Pxx_);
//	 write_csv("/home/mylord/esp_pf/data/result/mPxx.csv",convert_double(Pxx_,1,NUM_VAR*NUM_VAR,-1),NUM_VAR,NUM_VAR);

	// Rmat and msmt_prcs
	msmt msmtinfo = msmt_prcs(obs);
	Mat_S Rmat = R_cal(msmtinfo.n_aoa,msmtinfo.n_tdoa);
	fixed_type R [N_MEAS];
	for(int i=0; i < N_MEAS;i++)
	{
		R[i] = Rmat.entries[i*NUM_VAR+i];
	}

	// CalPzxZdiff-----------------------//
	fixed_type zDiff[NUM_PARTICLES*N_MEAS];
	fixed_type pzx[NUM_PARTICLES*N_MEAS*N_MEAS];
	CalPzxZdiff(prtcls,&msmtinfo,R,step,Pxx_,zDiff,pzx);

	// mvnpdf---------------------------//
	fixed_type sum_fp =0;
	double N_eff=0;
	int n_obs = msmtinfo.n_aoa + msmtinfo.n_tdoa;
	// cout << "n_obs = " << n_obs << "\n";
	for(int i=0; i <NUM_PARTICLES;i++)
	{
		double zDiff_du[N_MEAS];
		double pzx_du[N_MEAS][N_MEAS];
		double Mu[N_MEAS] = {0,0,0,0,0,0};
		for(int i1=0; i1< N_MEAS;i1++)
		{
			zDiff_du[i1] = zDiff[i*N_MEAS+i1];
			for(int i2=0; i2< N_MEAS;i2++)
			{
				int i3 = i*N_MEAS*N_MEAS + i1*N_MEAS+i2;
				int i3_transpose = i*N_MEAS*N_MEAS + i2*N_MEAS+i1;
			    fixed_type tmp = (pzx[i3] + pzx[i3_transpose])/((fixed_type) 2.0);
				pzx_du[i1][i2]  = tmp;
			}
		}
		double p_du = mvnpdf_code(zDiff_du, Mu,pzx_du,n_obs);
//		double p_du = pzx_du[0][0];
		fixed_type p_fp = (isnan(p_du)?0:p_du);
		wt[i] = wt[i]*p_fp;
		sum_fp = sum_fp + wt[i];
	}

//	write_csv("/home/mylord/esp_pf/data/result/zDiff_cpp.csv",convert_double(zDiff,1,6*1024,-1),1024,6);
//	write_csv("/home/mylord/esp_pf/data/result/pzx_cpp.csv",convert_double(pzx,1,36*1024,-1),6*1024,6);
//	cout << "\n wt = " << wt[0] << ", " << wt[1] << "\n";

	for(int i =0; i < NUM_PARTICLES;i++)
	{
		if(sum_fp == 0){
			wt[i]  = 1023;
		}
		else{
			wt[i] = wt[i]/sum_fp;
		}
	}

	double re_sum =0;
	for(int j =0; j < NUM_PARTICLES;j++)
	{
		double temp2 = wt[j];
		re_sum += (temp2*temp2);
	}
	N_eff = 1/re_sum;


	if(N_eff < NUM_PARTICLES*0.5)
	{
		double rnd_temp;
		rnd_temp = RNG_withSeed(0,0);

		fixed_type rnd_rsmp = rnd_temp;
		resamplePF_wrap(prtcls,wt,rnd_rsmp);
		// cout << "resample required\n";
	}
	PFupdate(prtcls,wt,state,pxx);
}
