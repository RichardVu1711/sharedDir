/*******************************************************************************
Vendor: Xilinx
Associated Filename: vadd.cpp
Purpose: VITIS vector addition

*******************************************************************************
Copyright (C) 2019 XILINX, Inc.

This file contains confidential and proprietary information of Xilinx, Inc. and
is protected under U.S. and international copyright and other intellectual
property laws.

DISCLAIMER
This disclaimer is not a license and does not grant any rights to the materials
distributed herewith. Except as otherwise provided in a valid license issued to
you by Xilinx, and to the maximum extent permitted by applicable law:
(1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX
HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
in contract or tort, including negligence, or under any other theory of
liability) for any loss or damage of any kind or nature related to, arising under
or in connection with these materials, including for any direct, or any indirect,
special, incidental, or consequential loss or damage (including loss of data,
profits, goodwill, or any type of loss or damage suffered as a result of any
action brought by a third party) even if such damage or loss was reasonably
foreseeable or Xilinx had been advised of the possibility of the same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any
application requiring fail-safe performance, such as life-support or safety
devices or systems, Class III medical devices, nuclear facilities, applications
related to the deployment of airbags, or any other applications that could lead
to death, personal injury, or severe property or environmental damage
(individually and collectively, "Critical Applications"). Customer assumes the
sole risk and liability of any use of Xilinx products in Critical Applications,
subject only to applicable laws and regulations governing limitations on product
liability.

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
ALL TIMES.

*******************************************************************************/
#define OCL_CHECK(error, call)                                                                   \
    call;                                                                                        \
    if (error != CL_SUCCESS) {                                                                   \
        printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
        exit(EXIT_FAILURE);                                                                      \
    }


// This PS/PL v1.5 optimises the algorithmetic level of calculate GISPZx (particularly, pzx matrix).
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <random>


#include "vadd.h"
#include "global_define/global_define.h"
#include "global_define/read_write_csv.h"
#include "global_define/mat_lib.h"
#include "random_generator/normrnd.h"
#include "random_generator/RNG_withSeed.h"
#include "resample_pf/resample_pf.h"
#include "calweights/Calweights.h"
#include "rk4/rk4.h"
#include "global_define/GISmsmt_prcs.h"



std::vector<cl::Device> devices ;
cl::Device device;
cl::Context context;
cl::Program program;
std::vector<cl::Platform> platforms;
cl::CommandQueue q[Q_LEN];



int main(int argc, char* argv[]) {

    std::string xclbinFilename = argv[1];
    cl_int err;


    //DEVICE INITIALISATION
    device_setup(argc,argv,devices,device,context,q,program);

    cout << "Init Phase: Create Kernel";

    OCL_CHECK(err, cl::Kernel kPFU = cl::Kernel(program,"PFupdate", &err));
    OCL_CHECK(err, cl::Kernel kCal = cl::Kernel(program,"CalPzxZdiff", &err));
    OCL_CHECK(err, cl::Kernel k_mPxx = cl::Kernel(program,"mean_Pxx", &err));
    OCL_CHECK(err, cl::Kernel kSigma = cl::Kernel(program,"sigmaComp", &err));
    OCL_CHECK(err, cl::Kernel kCreate = cl::Kernel(program,"ESPCrtParticles", &err));


    // This call will get the kernel object from program. A kernel is an
    // OpenCL function that is executed on the FPGA.
    // These commands will allocate memory on the Device. The cl::Buffer objects can
    // be used to reference the memory locations on the device.


	// Compute the size of array in bytes

    cout << "Phase: Create buffer\n";
    // input buffers initialized
    OCL_CHECK(err, cl::Buffer b_stateIn(context, CL_MEM_READ_ONLY, size_state, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_sigMatIn(context, CL_MEM_READ_ONLY, size_large, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_rndIn(context, CL_MEM_READ_ONLY, size_large, NULL, &err));

    OCL_CHECK(err, cl::Buffer b_prtclsIn(context, CL_MEM_READ_ONLY, size_large, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_prtclsIn2(context, CL_MEM_READ_ONLY, size_large, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_msmtIn(context, CL_MEM_READ_ONLY, size_msmt, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_RmatIn(context, CL_MEM_READ_ONLY, size_Rmat, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_pxxIn(context, CL_MEM_READ_ONLY, size_pxx, NULL, &err));

    OCL_CHECK(err, cl::Buffer b_wtIn(context, CL_MEM_READ_ONLY, size_wt, NULL, &err));
    // output buffers initialized


    OCL_CHECK(err, cl::Buffer b_sigMatOut(context, CL_MEM_READ_ONLY, size_large, NULL, &err));

    OCL_CHECK(err, cl::Buffer b_prtclsOut(context, CL_MEM_READ_ONLY, size_large, NULL, &err));

    OCL_CHECK(err, cl::Buffer b_zDiffOut(context, CL_MEM_WRITE_ONLY, size_zDiff, NULL, &err));
	OCL_CHECK(err, cl::Buffer b_pzxOut(context, CL_MEM_WRITE_ONLY, size_pzx, NULL, &err));

    OCL_CHECK(err, cl::Buffer b_stateOut(context, CL_MEM_WRITE_ONLY, size_state, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_pxxOut(context, CL_MEM_WRITE_ONLY, size_pxx, NULL, &err));

    //set the kernel Arguments

    cout << "Phase: set kernel arguments for\n";
    OCL_CHECK(err, err = kCreate.setArg(0,b_stateIn));
	OCL_CHECK(err, err = kCreate.setArg(1,b_sigMatIn));
	OCL_CHECK(err, err = kCreate.setArg(2,b_prtclsOut));

    OCL_CHECK(err, err = kSigma.setArg(0,b_pxxIn));
    OCL_CHECK(err, err = kSigma.setArg(1,b_sigMatOut));
    OCL_CHECK(err, err = kSigma.setArg(2,b_rndIn));

    OCL_CHECK(err, err = k_mPxx.setArg(0,b_prtclsIn));
    OCL_CHECK(err, err = k_mPxx.setArg(1,b_pxxOut));

    OCL_CHECK(err, err = kCal.setArg(0,b_prtclsIn));
    OCL_CHECK(err, err = kCal.setArg(1,b_msmtIn));
    OCL_CHECK(err, err = kCal.setArg(2,b_RmatIn));
    OCL_CHECK(err, err = kCal.setArg(3,0));
    OCL_CHECK(err, err = kCal.setArg(4,b_pxxIn));
    OCL_CHECK(err, err = kCal.setArg(5,b_zDiffOut));
    OCL_CHECK(err, err = kCal.setArg(6,b_pzxOut));

    OCL_CHECK(err, err = kPFU.setArg(0,b_prtclsIn2));
    OCL_CHECK(err, err = kPFU.setArg(1,b_wtIn));
    OCL_CHECK(err, err = kPFU.setArg(2,b_stateOut));
    OCL_CHECK(err, err = kPFU.setArg(3,b_pxxOut));

    //We then need to map our OpenCL buffers to get the pointers
    int *p_rndIn,*p_stateIn,*p_sigMatIn,*p_prtclsIn,*p_prtclsIn2,*p_msmtIn,*p_RmatIn,*p_pxxIn,*p_wtIn,
		*p_sigMatOut,*p_prtclsOut,*p_zDiff,*p_pzx,*p_stateOut,*p_pxxOut;
    cout << "Phase: Link Pointer\n";


	// link input pointer
	buf_link(&p_rndIn,b_rndIn,size_large,WBUF,1);
	buf_link(&p_stateIn,b_stateIn,size_state,WBUF,1);
	buf_link(&p_sigMatIn,b_sigMatIn,size_large,WBUF,0);
	buf_link(&p_prtclsIn,b_prtclsIn,size_large,WBUF,1);
	buf_link(&p_prtclsIn2,b_prtclsIn2,size_large,WBUF,1);

	buf_link(&p_msmtIn,b_msmtIn,size_msmt,WBUF,0);
	buf_link(&p_RmatIn,b_RmatIn,size_Rmat,WBUF,1);
	buf_link(&p_pxxIn,b_pxxIn,size_pxx,WBUF,1);
	buf_link(&p_wtIn,b_wtIn,size_wt,WBUF,1);

	// link output pointer
	buf_link(&p_sigMatOut,b_sigMatOut,size_large,RBUF,0);
	buf_link(&p_prtclsOut,b_prtclsOut,size_large,RBUF,1);
	buf_link(&p_pxxOut,b_pxxOut,size_pxx,RBUF,0);
	buf_link(&p_zDiff,b_zDiffOut,size_zDiff,RBUF,1);
	buf_link(&p_pzx,b_pzxOut,size_pzx,RBUF,0);
	buf_link(&p_stateOut,b_stateOut,size_state,RBUF,0);
    cout << "Phase: Start ESP-PF\n";

    system(" rm -rf /mnt/result/*.csv");
    cl::Event status_S,status_S1;

    // prepare for three stages

    for(int i_run = 0; i_run < 100;i_run++)
    {
    	fixed_type Gstate[N_SRC][NUM_VAR];
    	fixed_type Gpxx[N_SRC][NUM_VAR*NUM_VAR];
    	fixed_type Gwt[N_SRC][NUM_PARTICLES];
    	fixed_type Gobs[N_SRC][10];
    	fixed_type G_cAvg[N_SRC][N_MEAS];
    	fixed_type G_nAvg[N_SRC][N_MEAS];

        fixed_type prtcls[NUM_VAR*NUM_PARTICLES];

        fixed_type wt[1*NUM_PARTICLES];
        double N_eff =0;
        fixed_type obs_data[N_SRC][N_OBS*10];

        convert_FP(read_csvMulLine("/mnt/test_data/obsVal1/Init/obsVal1.csv",0, N_OBS, 10),
        		obs_data[0], 1, N_OBS*10, -1);
        convert_FP(read_csvMulLine("/mnt/test_data/obsVal2/Init/obsVal2.csv",0, N_OBS, 10),
        		obs_data[1], 1, N_OBS*10, -1);
//        events.mark("begin");

        // there are states for controlling the FSM of state
	    state_t pstate[N_SRC]; // keep tracking of state at computing state (can't be update at IDLE/SLEEP state)
	    state_t cstate[N_SRC]; // current state will be updated with nstate at the end of FSM.
	    state_t nstate[N_SRC]; // next state will be modified at the end of blocks
//	    xrt::profile::user_range profiler[N_SRC];
		for(int i_step=0; i_step <N_OBS ;i_step++)
		{

			int s_stt = 0;
			int c_stt = 0;
			int r_stt = 0;
			int Sinit = 0; // indicating initalisation in this block.
			int Cinit = 0; // indicating initalisation in this block.
			int Rinit = 0; // indicating initalisation in this block.

			int idone[N_SRC];
			fixed_type prtclsTmp[NUM_PARTICLES*NUM_VAR];
			for(int i=0; i < N_SRC;i++)
			{
				// idone is used to check if the initialisation is conducted or not.
				idone[i] = 0;
			}
			samp_state_t pbS = SINIT;
			samp_state_t nbS = SINIT;
			samp_state_t pbC = SINIT;
			samp_state_t nbC = SINIT;
			samp_state_t pbR = SINIT;
			samp_state_t nbR = SINIT;
		    for(int i=0; i < N_SRC;i++)
		    {
		    	pstate[i] = INIT;
		    	cstate[i] = INIT;
		    	nstate[i] = INIT;

		    }
			int timer =0;
			cl::Event done_S;
			cl::Event done_C;
			cl::Event done_R;
			int n_meas[N_SRC];
			// conduct an initialisation
			do{
				for(int idx_s = 0; idx_s < N_SRC;idx_s++){
//				for(int idx_s = N_SRC-1; idx_s >=0;idx_s--){
				switch(cstate[idx_s]){
				case INIT:{
					pstate[idx_s] = cstate[idx_s];
//					cout << "\nsrc " << idx_s << " is at INIT " <<".\n";
					if(idone[idx_s] ==0){
						idone[idx_s] =1;
						for(int i=0; i < 10;i++){
							Gobs[idx_s][i] = obs_data[idx_s][i_step*10+i];
						}
						if(i_step==0){
							cout << "Phase: initialisation 2\n";
							// only read the first one
							string  state_dir = "/mnt/test_data/obsVal"+to_string(idx_s+1) +"/Init/state_in.csv";
							string  pxx_dir = "/mnt/test_data/obsVal"+to_string(idx_s+1) +"/Init/Pxx_in.csv";

							convert_FP(read_csvMulLine(state_dir,0*NUM_VAR, NUM_VAR, 1),
														&Gstate[idx_s][0], 1, NUM_VAR, -1);
							convert_FP(read_csvMulLine(pxx_dir,0*NUM_VAR, NUM_VAR, NUM_VAR),
														&Gpxx[idx_s][0], NUM_VAR, NUM_VAR, 0);
							for(int i=0; i < NUM_PARTICLES;i++){
								Gwt[idx_s][i] = 1.0/NUM_PARTICLES;
							}
							for(int i=0; i < N_MEAS;i++){
								// initialise the current average to obs
								G_cAvg[idx_s][i] = Gobs[idx_s][i];
							}
						}
					}
					else{
						// check if the next block is ready to be occupied.
						if(s_stt == 0){
							if(idx_s ==0){
								nstate[idx_s] = SAMP;
							}
							else{
								if(pstate[idx_s-1]==CAL){
									// the next block - SAMP is available as the previous source is in CAL
									nstate[idx_s] = SAMP;
									pstate[idx_s] = SAMP;
								}
								else{
									nstate[idx_s] = IDLE;
								}
							}
						}
						else nstate[idx_s] = IDLE;
					}
				}
				break;
				case SAMP:{
					//wait in here until the block is finished
					//or keep executing
					// only update the state in these block indicating the current track.
					pstate[idx_s] = cstate[idx_s];
					int tmp = s_stt;
//					std::string state_dir = "/mnt/result/stateIn" + to_string(idx_s) + ".csv";
//					std::string pxx_dir = "/mnt/result/pxxIn" + to_string(idx_s) + ".csv";
//					write_csv(state_dir,convert_double(&Gstate[idx_s][0],1,13,-1),1,13);
//					write_csv(pxx_dir,convert_double(&Gpxx[idx_s][0],13,13,-1),13,13);
					// there should be no writing into Global Memory in this phase !!!
					// all global memory should be read-only at this stage.
					s_stt = block_S(&p_pxxIn,&Gpxx[idx_s][0], b_pxxIn,
						&p_stateIn,&Gstate[idx_s][0],b_stateIn,
						&p_sigMatIn,b_sigMatIn,
						&p_sigMatOut, b_sigMatOut,
						&p_rndIn,b_rndIn,
						&p_prtclsOut,b_prtclsOut,prtcls,prtclsTmp,
						&p_prtclsIn,b_prtclsIn,
						&p_pxxOut,b_pxxOut,
						kSigma,kCreate, k_mPxx,
						&nstate[idx_s], &pbS,&nbS,
						&Sinit, s_stt, c_stt,
						idx_s,&done_S,
						i_step,i_run);
					if(s_stt !=4){
						// as we already update the next state in P4,
						// hence no need to overwrite again
						if(pbS==SWAIT) nstate[idx_s] = IDLE;
						else{
							nstate[idx_s] = SAMP;
//							memcpy(prtclsTmp,p_prtclsOut,size_large);
						}
					}
				}

				break;
				case CAL:{
					pstate[idx_s] = cstate[idx_s];
					c_stt = block_C(prtcls,
									&p_pxxOut,&p_pxxIn,b_pxxIn,
									&p_msmtIn,b_msmtIn, &n_meas[idx_s],
									&p_RmatIn, b_RmatIn,
									&Gobs[idx_s][0],
									&p_zDiff, b_zDiffOut,
									&p_pzx,b_pzxOut,
									i_step,kCal,
									&nstate[idx_s], &pbC, &nbC,
									&Cinit,c_stt,r_stt,&s_stt,
									idx_s,&done_C,
									G_cAvg[idx_s],G_nAvg[idx_s]);
					if(c_stt != 2){
						// as we already update the next state in P2,
						// hence no need to overwrite again
						if(pbC==SWAIT) nstate[idx_s] = IDLE;
						else nstate[idx_s] = CAL;
					}
				}
				break;
				case UP:{
				pstate[idx_s] = cstate[idx_s];
				r_stt =block_R(prtcls,&Gwt[idx_s][0],
						n_meas[idx_s],
						&p_zDiff,&p_pzx,
						&p_prtclsIn2,b_prtclsIn2,
						&p_wtIn,b_wtIn,
						&p_stateOut, b_stateOut,
						&p_pxxOut, b_pxxOut,
						&Gstate[idx_s][0],&Gpxx[idx_s][0],
						i_step,
						kPFU,
						&nstate[idx_s], &pstate[idx_s],&pbR,&nbR,
						&Rinit,r_stt, &c_stt,
						idx_s, &done_R,i_run);
				if(r_stt != 2){
					// as we already update the next state in P2,
					// hence no need to overwrite again
					if(pbR==SWAIT) nstate[idx_s] = IDLE;
					else nstate[idx_s] = UP;
				}
//				if(timer%1==0)	print_state(idx_s,nstate[idx_s],pbR);
				}
				break;

//					cout << "\nsrc " << idx_s << " is at block_R, phase " << r_stt << "," << src_state[idx_s]<< ".\n";
//
//					break;
				case IDLE:{
					// check if the any interrupts
					if(pstate[idx_s]!=IDLE){
					switch(pstate[idx_s]){
					case SAMP:{
						if(done_S() == NULL){
							// event isNull
							// an event is Null then check if this stage =0 or 4
							if((s_stt ==0)||((s_stt ==4)&&(c_stt==0))){
								nstate[idx_s] = SAMP;
							}
							else{
								// keep sleeping
								nstate[idx_s] = IDLE;
							}
						}
						else{
							// there is something in the event
							// as if there is something in this event => current state must be P1 or P2
							// now we want to check if the event is finished
							// This will be replaced with interrupt in the future
							if((s_stt == 1)||(s_stt ==2)||(s_stt ==3)){
								cl_int pDone;
								OCL_CHECK(err, err = done_S.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pDone));
								if(pDone == CL_COMPLETE)	nstate[idx_s] = SAMP;
								else nstate[idx_s] = IDLE;	// wait for KSigma Finished.
//									nstate[idx_s] = SAMP;
							}
							else{
								cout << "\n Unexpected at IDLE while waiting for SAMP\n";
								return -1;
							}
						}
					}
					break;
					case CAL:{
//							cout << "\n IDLE while waiting for CAL\n";
						if(done_C() == NULL){
							// no events => either initialisation or final stage
							if((c_stt ==0)||(c_stt ==2)){
								nstate[idx_s] = CAL;
							}
							else{
								// a safe lock to make sure that no thing unexpected
								// keep sleeping
								nstate[idx_s] = IDLE;
							}
						}
						else{
							if(c_stt == 1){
								cl_int pDone;
								OCL_CHECK(err, err = done_C.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pDone));
								if(pDone == CL_COMPLETE)	nstate[idx_s] = CAL;
								else nstate[idx_s] = IDLE;	// wait for KSigma Finished.
							}
							else{
								cout << "\n Unexpected at IDLE while waiting for SAMP\n";
								return -1;
							}
						}
					}
					break;
					case UP:{
						if(done_R() == NULL){
							// no events => either initialisation or final stage
							if((r_stt ==0)||(r_stt ==2)){
								nstate[idx_s] = UP;
							}
							else{
								// a safe lock to make sure that no thing unexpected
								// keep sleeping
								nstate[idx_s] = IDLE;
							}
						}
						else{
							if(r_stt == 1){
								cl_int pDone;
								OCL_CHECK(err, err = done_R.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pDone));
								if(pDone == CL_COMPLETE)	{
									nstate[idx_s] = UP;
								}
								else nstate[idx_s] = IDLE;	// wait for KSigma Finished.
							}
							else{
								cout << "\n Unexpected at IDLE while waiting for UP\n";
								return -1;
							}
						}
					}
					break;
					case INIT:{
						if(s_stt == 0){
							if(idx_s ==0)
								nstate[idx_s] = SAMP;
							else{
								// idx_s != 0;
								// first source is using it.
								if(pstate[idx_s-1]==CAL){
//									cout << "idx = "<< idx_s<< ", state = "<< pstate[idx_s-1] <<"\n";
									nstate[idx_s] = SAMP;
									pstate[idx_s] = SAMP;
								}
								else{
									nstate[idx_s] = IDLE;
								}
							}
//							cout << "idx = "<< idx_s<< ", state = "<< cstate[idx_s] <<"\n";
						}
					}
					break;
					default:{
						cout << "A random stage \n";
						return -1;
					}
					}
				}
				else
				{

//					if(timer%1000==0){
//	//					cout << "\n";
//
//					print_state(idx_s,pstate[idx_s],timer);
//					}
//					return -1;
				}
				}
				break;
					// Update state
				}
				cstate[idx_s] = nstate[idx_s];
				}
				timer++;
			}
//			while(evaluate_step(src_state));
			while(pstate[N_SRC-1]!= IDLE);
			if(timer > 10000){
				system("clear");
				timer = 0;
			}
		}
	    cout << "\nEND OF RUN: " << i_run << "\n";

    }
//    events.mark("end");
    for(int i=0; i < N_SRC;i++)
    {
        OCL_CHECK(err, err = q[i].finish());
    }
    cout << "\nDone. Good Luck with the Result, My Lord \n";

    OCL_CHECK(err, err = q[0].finish());
    OCL_CHECK(err, err = q[1].finish());
    for(int i=0; i < N_SRC;i++){
        clReleaseCommandQueue(q[i]());

    }
//    clReleaseCommandQueue(q[2]());

    clReleaseContext(context.get());
    clReleaseDevice(device.get());
    clReleaseProgram(program.get());
//    free(platforms[0].getInfo<CL_PLATFORM_NAME>());
//    free(Device_IDs);
//
//    clReleaseKernel(Kernel);



    int match =0;

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);
}

bool evaluate_step(state_t src_state[N_SRC])
{
//	bool result = true;
	for(int i=0; i < N_SRC;i++)
	{
//		cout << "\nsrc_state " << i << " =" << src_state[i] <<"\n";
		if(src_state[i] != IDLE)
			return true;
	}
	return false;
}

void print_state(int idx, state_t cstate, int phase)
{
	cout << "Source " << idx << " is at state ";
	switch(cstate)
	{
	case INIT:
			cout << "INIT phase=" << phase << "\n";
		break;
	case SAMP:
		cout << "SAMP phase=" << phase << "\n";
		break;
	case CAL:
		cout << "CAL phase=" << phase << "\n";
		break;
	case UP:
		cout << "UP phase=" << phase << "\n";
		break;
	case IDLE:
		cout << "IDLE phase=" << phase << "\n";
		break;
	}
}

