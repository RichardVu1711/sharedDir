#include "../Calweights.h"
#include "../ESPCrtParticles.h"
#include "../mean_pxx.h"
#include "../PFupdate.h"
#include "../sigmaComp.h"
#include "../lib/read_write_csv.h"
#include "../resample_pf/resample_pf.h"
#include "../random_generator/RNG_withSeed.h"
#include "../rk4/rk4.h"
#include "../calW/mvnpdf_code.h"

#define DATA_PATH "C:/ESP_PF_PLNewWeight/AXI_ESP_PF"
#include <algorithm>
#include <chrono>
#include <iostream>
#include<vector>

#include <stdlib.h>
#include <fstream>
#include <iostream>

#include <random>
#define N_OBS 350
using namespace std::chrono;
//int main1()
//{
//	cout << "Phase: initialisation \n";
//
//	rand_init(0);
//	double N_eff =0;
//
//	fixed_type prtcls[NUM_VAR*NUM_PARTICLES];
//	fixed_type state[NUM_VAR];
//	fixed_type pxx[NUM_VAR*NUM_VAR];
//	fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES];
//	fixed_type rnd_rk4[NUM_VAR*4];
//	fixed_type sigMat[NUM_VAR*NUM_PARTICLES];
//	fixed_type wt[1*NUM_PARTICLES];
//	fixed_type obs_data[350*10];
//	fixed_type Pxx_[NUM_VAR*NUM_VAR];
//	cout << "Phase: initialisation \n";
//	convert_FP(read_csvMulLine("C:/ESP_PF_PLNewWeight/test_data/obsVals_missing1/Init/obsVals1.csv",0, N_OBS, 10),
//			obs_data, 1, N_OBS*10, -1);
//	for(int i=0; i < 10;i++){
//		cout << obs_data[0*10+i] << ", ";
//	}
//    Mat_S obs;
//    double eff_[N_OBS];
//    init_mat(&obs,1,10);
////    system(" del C:\ESP_PF_PLNewWeight\AXI_ESP_PF\result\*.csv");
//
//
//	for(int i_run = 0; i_run < 1;i_run++)
//	{
//		for(int i_step=0; i_step < N_OBS;i_step++)
//		{
//			for(int i=0; i < 1;i++){
//				obs.entries[i] = obs_data[i_step*10+i];
//			}
//			if(i_step==0){
//				cout << "Phase: initialisation \n";
//				// only read the first one
////				convert_FP(read_csvMulLine("C:/ESP_PF_PLNewWeight/test_data/obsVals_missing1/Init/prtcls_init.csv",0, NUM_VAR, NUM_PARTICLES),
////											prtcls, NUM_VAR, NUM_PARTICLES, 1);
//				convert_FP(read_csvMulLine("C:/ESP_PF_PLNewWeight/test_data/obsVals_missing1/Init/state_init1.csv",0*NUM_VAR, NUM_VAR, 1),
//											state, 1, NUM_VAR, -1);
//				convert_FP(read_csvMulLine("C:/ESP_PF_PLNewWeight/test_data/obsVals_missing1/Init/Pxx_in.csv",0*NUM_VAR, NUM_VAR, NUM_VAR),
//											pxx, NUM_VAR, NUM_VAR, 0);
//				for(int i=0; i < NUM_PARTICLES;i++)
//				{
//					wt[i] = 1.0/NUM_PARTICLES;
//				}
//			}
//
//
//			for(int i=0; i< NUM_VAR*NUM_PARTICLES;i++)
//			{
//				double rnd_temp;
//				randn(&rnd_temp,0,0);
//				rnd_sigma[i] = rnd_temp;
//			}
//
//			for(int i=0; i< NUM_VAR*4;i++)
//			{
//				double rnd_temp;
//				randn(&rnd_temp,0,0);
//				rnd_rk4[i] = rnd_temp;
//			}
//
//			// rk4 ------------------------------------//
//			Mat_S stateOut_pro;
//			Mat_S M_state;
//			M_state.col = 1;
//			M_state.row = NUM_VAR;
//			for(int i=0; i < 13;i++)
//			{
//				M_state.entries[i*NUM_VAR] = state[i];
//			}
//
//			rk4(&M_state,&stateOut_pro,rnd_rk4);
//			// sigmaComp ----------------------------//
//			sigmaComp(pxx, sigMat,rnd_sigma);
//			fixed_type state_pro[NUM_VAR];
//			for(int i=0; i < NUM_VAR;i++)
//			{
//				state_pro[i] = stateOut_pro.entries[i*NUM_VAR];
//			}
//
//			// ESPCrtParticles ---------------------//
//			ESPCrtParticles(state_pro,sigMat,prtcls);
//
//			write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/sigMat.csv",convert_double(sigMat,1,13*1024,0),13,1024);
//			write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/prtcls.csv",convert_double(prtcls,1,13*1024,0),13,1024);
//
//			// mean_Pxx --------------------------//
//			mean_Pxx(prtcls,Pxx_);
//			// Rmat and msmt_prcs
//			msmt msmtinfo = msmt_prcs(&obs);
//			Mat_S Rmat = R_cal(msmtinfo.n_aoa,msmtinfo.n_tdoa);
//			fixed_type R [N_MEAS];
//			for(int i=0; i < N_MEAS;i++)
//			{
//				R[i] = Rmat.entries[i*NUM_VAR+i];
//			}
//			// CalPzxZdiff-----------------------//
//			fixed_type zDiff[NUM_PARTICLES*N_MEAS];
//			fixed_type pzx[NUM_PARTICLES*N_MEAS*N_MEAS];
//			CalPzxZdiff(prtcls,&msmtinfo,R,i_step,Pxx_,zDiff,pzx);
//
//			// mvnpdf---------------------------//
//			fixed_type sum_fp =0;
//			double N_eff=0;
//			int n_obs = msmtinfo.n_aoa + msmtinfo.n_tdoa;
//			for(int i=0; i <NUM_PARTICLES;i++)
//			{
//				double zDiff_du[N_MEAS];
//				double pzx_du[N_MEAS][N_MEAS];
//				double Mu[N_MEAS] = {0,0,0,0,0,0};
//				for(int i1=0; i1< N_MEAS;i1++)
//				{
//					zDiff_du[i1] = zDiff[i*N_MEAS+i1];
//					for(int i2=0; i2< N_MEAS;i2++)
//					{
//						pzx_du[i1][i2] = pzx[i*N_MEAS*N_MEAS + i1*N_MEAS+i2];
//					}
//				}
//				double p_du = mvnpdf_code(zDiff_du, Mu,pzx_du,n_obs);
//
//				fixed_type p_fp = p_du;
//				wt[i] = wt[i]*p_fp;
//				sum_fp = sum_fp + wt[i];
//				cout << p_du <<", ";
//			}
//
////			for(int i =0; i < NUM_PARTICLES;i++)
////			{
////				wt[i] = wt[i]/sum_fp;
////			}
////
////			double re_sum =0;
////			for(int j =0; j < NUM_PARTICLES;j++)
////			{
////				double temp2 = wt[j];
////				re_sum += (temp2*temp2);
////			}
////			N_eff = 1/re_sum;
//
////			write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/obs2/zDiff.csv",convert_double(zDiff,1,6*1024,-1),1024,6);
////			write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/obs2/pzx.csv",convert_double(pzx,1,36*1024,-1),6*1024,6);
////			write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/obs2/wt.csv",convert_double(wt,1,1*1024,-1),1,1024);
////
////			if(N_eff < NUM_PARTICLES*0.5 || N_eff > DBL_MAX*0.5)
////			{
////				double rnd_temp;
////				randn(&rnd_temp,0,0);
////				fixed_type rnd_rsmp = rnd_temp;
////				resamplePF_wrap(prtcls,wt,rnd_rsmp);
////				cout << "resample required\n";
////			}
//////			void PFupdate(fixed_type particle[NUM_VAR*NUM_PARTICLES],
//////						fixed_type wt[NUM_PARTICLES],
//////						fixed_type state[NUM_VAR],
//////						fixed_type pxx[NUM_VAR*NUM_VAR])
////			PFupdate(prtcls,wt,state,pxx);
////			eff_[i_step] = N_eff;
////			write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/state_sol.csv",convert_double(state,1,13,-1),1,13);
////			write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/pxx_sol.csv",convert_double(pxx,1,13*13,-1),13,13);
////
////			cout << "\nstate= " << state[0] << ", " << state[1] << "\t pxx =" << pxx[0] << ", " << pxx[14] << "\n";
////			cout << "End of : " << i_step << " with Neff =  "<< N_eff <<".\n";
////
//		}
//////		std::vector<double> out_du;
//////		for(int i=0; i < 52;i++)
//////		{
//////			out_du.at(i) =  eff_[i];
//////		}
//////		write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/Neff.csv",out_du,1,52);
//
//	}
//
//	// auto const seed = 0;
//	// auto urbg = std::mt19937 {seed};
//	// double const mu = 0.0;
//	// double const sigma = 1.0;
//	// auto norm = std::normal_distribution<double>{mu,sigma};
//	// std::vector<double> randn_;
//	// for(int i=0; i< NUM_VAR*NUM_PARTICLES;i++){
//	// 	double rnd_temp = norm(urbg);
//	// 	randn_.push_back( rnd_temp);
//	// }
//	// write_csv("C:/ESP_PF_PLNewWeight/AXI_ESP_PF/result/rnd_temp.csv",randn_,1,13*1024);
//
//	return 0;
//}
