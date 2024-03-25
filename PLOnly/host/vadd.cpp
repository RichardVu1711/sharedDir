///*******************************************************************************
//Vendor: Xilinx
//Associated Filename: vadd.cpp
//Purpose: VITIS vector addition
//
//*******************************************************************************
//Copyright (C) 2019 XILINX, Inc.
//
//This file contains confidential and proprietary information of Xilinx, Inc. and
//is protected under U.S. and international copyright and other intellectual
//property laws.
//
//DISCLAIMER
//This disclaimer is not a license and does not grant any rights to the materials
//distributed herewith. Except as otherwise provided in a valid license issued to
//you by Xilinx, and to the maximum extent permitted by applicable law:
//(1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX
//HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
//INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR
//FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
//in contract or tort, including negligence, or under any other theory of
//liability) for any loss or damage of any kind or nature related to, arising under
//or in connection with these materials, including for any direct, or any indirect,
//special, incidental, or consequential loss or damage (including loss of data,
//profits, goodwill, or any type of loss or damage suffered as a result of any
//action brought by a third party) even if such damage or loss was reasonably
//foreseeable or Xilinx had been advised of the possibility of the same.
//
//CRITICAL APPLICATIONS
//Xilinx products are not designed or intended to be fail-safe, or for use in any
//application requiring fail-safe performance, such as life-support or safety
//devices or systems, Class III medical devices, nuclear facilities, applications
//related to the deployment of airbags, or any other applications that could lead
//to death, personal injury, or severe property or environmental damage
//(individually and collectively, "Critical Applications"). Customer assumes the
//sole risk and liability of any use of Xilinx products in Critical Applications,
//subject only to applicable laws and regulations governing limitations on product
//liability.
//
//THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
//ALL TIMES.
//
//*******************************************************************************/
//#define OCL_CHECK(error, call)                                                                   \
//    call;                                                                                        \
//    if (error != CL_SUCCESS) {                                                                   \
//        printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
//        exit(EXIT_FAILURE);                                                                      \
//    }
//
//
// This PS/PL v1.5 optimises the algorithmetic level of calculate GISPZx (particularly, pzx matrix).
#include <stdlib.h>
#include <fstream>
#include <iostream>

//#include "experimental/xrt_profile.h"
//#include "xcl2.hpp"
//#include "experimental/xrt_profile.h"

#include "vadd.h"
#include "lib/global_define.h"
#include "lib/read_write_csv.h"
#include "lib/mat_lib.h"
#include "lib/GISmsmt_prcs.h"
#include "env_setup/host_setup.h"

#define N_OBS 1

std::vector<cl::Device> devices ;
cl::Device device;
cl::Context context;
cl::Program program;
std::vector<cl::Platform> platforms;
cl::CommandQueue q[Q_LEN];


int main(int argc, char* argv[]) {

//  xrt::profile::user_range range("Phase 0", "Intialization");

//	xrt::profile::user_event events;
//
//	std::string binaryFile = argv[1];
    std::string xclbinFilename = argv[1];
    cl_int err;

    //DEVICE INITIALISATION
    device_setup(argc,argv,devices,device,context,q,program);

    cout << "Init Phase: Create Kernel";

    OCL_CHECK(err, cl::Kernel kESP = cl::Kernel(program,"ESP_PF_Wrapper", &err));
//    xrt::profile::user_range range("Init");
//    range.end();


    // This call will get the kernel object from program. A kernel is an
    // OpenCL function that is executed on the FPGA.
    // These commands will allocate memory on the Device. The cl::Buffer objects can
    // be used to reference the memory locations on the device.


	// Compute the size of array in bytes

    cout << "Phase: Create buffer\n";
    // input buffers initialized
    OCL_CHECK(err, cl::Buffer b_obs(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_stateIn(context, CL_MEM_READ_ONLY, size_state, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_pxxIn(context, CL_MEM_READ_ONLY, size_pxx, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_wtIn(context, CL_MEM_READ_ONLY, size_wt, NULL, &err));


    // output buffers initialized
    OCL_CHECK(err, cl::Buffer b_stateOut(context, CL_MEM_WRITE_ONLY, size_state, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_pxxOut(context, CL_MEM_WRITE_ONLY, size_pxx, NULL, &err));
    OCL_CHECK(err, cl::Buffer b_wtOut(context, CL_MEM_WRITE_ONLY, size_wt, NULL, &err));

    //set the kernel Arguments
    cout << "Phase: set kernel arguments for\n";
    OCL_CHECK(err, err = kESP.setArg(0,b_obs));
    OCL_CHECK(err, err = kESP.setArg(1,b_pxxIn));
    OCL_CHECK(err, err = kESP.setArg(2,b_stateIn));
    OCL_CHECK(err, err = kESP.setArg(3,b_wtIn));
    OCL_CHECK(err, err = kESP.setArg(4,b_pxxOut));
    OCL_CHECK(err, err = kESP.setArg(5,b_stateOut));
    OCL_CHECK(err, err = kESP.setArg(6,b_wtOut));


    //We then need to map our OpenCL buffers to get the pointers
    int *p_obs,*p_stateIn,*p_pxxIn,*p_wtIn,
		 *p_stateOut,*p_pxxOut,*p_wtOut;
    cout << "Phase: Link Pointer\n";


	// link input pointer
	buf_link(&p_obs,b_obs,size_Mat_S,WBUF,0);
	buf_link(&p_pxxIn,b_pxxIn,size_pxx,WBUF,0);
	buf_link(&p_stateIn,b_stateIn,size_state,WBUF,0);
	buf_link(&p_wtIn,b_wtIn,size_wt,WBUF,0);

	buf_link(&p_pxxOut,b_pxxOut,size_pxx,RBUF,0);
	buf_link(&p_stateOut,b_stateOut,size_state,RBUF,0);
	buf_link(&p_wtOut,b_wtOut,size_wt,RBUF,0);

    cout << "Phase: Start ESP-PF\n";

    system(" rm -rf /mnt/result/*.csv");
    for(int i_run = 0; i_run < 1;i_run++)
    {
    	Mat_S obs;
		fixed_type pxx_in[NUM_VAR*NUM_VAR];
		fixed_type state_In[NUM_VAR];
		fixed_type pxxOut[NUM_VAR*NUM_VAR];
		fixed_type stateOut[NUM_VAR];
		fixed_type wtOut[NUM_PARTICLES];
		fixed_type wt[NUM_PARTICLES];
		fixed_type obs_data[N_OBS*10];
		convert_FP(read_csvMulLine("/mnt/test_data/obsVal2/Init/obsVal2.csv",154, N_OBS, 10),
									obs_data, 1, N_OBS*10, -1);
		for(int i_step =0; i_step < N_OBS;i_step++){
//			range.start("ESP-PF");
			for(int i=0; i < 10;i++){
				obs.entries[i] = obs_data[i_step*10 + i];
			}
			obs.col = 10;
			obs.row = 1;
			if(i_step ==0){
				cout << "Read init Data \n";
				convert_FP(read_csvMulLine("/mnt/test_data/obsVal2/Init/state_in.csv",0*NUM_VAR, NUM_VAR, 1),
						state_In, 1, NUM_VAR, -1);
				convert_FP(read_csvMulLine("/mnt/test_data/obsVal2/Init/pxx_in.csv",0*NUM_VAR, NUM_VAR, NUM_VAR),
						pxx_in, NUM_VAR, NUM_VAR, 0);
				for(int i=0; i < NUM_PARTICLES;i++){
					wt[i] = 1.0/NUM_PARTICLES;
				}
				memcpy(p_pxxIn,pxx_in,size_pxx);
				memcpy(p_stateIn,state_In,size_state);
				memcpy(p_wtIn,wt,size_wt);
				memcpy(p_obs,&obs,size_Mat_S);

				cout << obs_data[9] << ", " << state_In[0] << ", " << pxx_in[0] << "\n";
			}
			else{
				memcpy(p_pxxIn,p_pxxOut,size_pxx);
				memcpy(p_stateIn,p_stateOut,size_state);
				memcpy(p_wtIn,p_wtOut,size_wt);
				memcpy(p_obs,&obs,size_Mat_S);
			}
//			ESP_PF_Wrapper(&obs,pxx_in,state_In,wt,pxxOut,stateOut,wtOut,i_step,0);
			//copy state out into state in
			// execute sigmaComp Block
			cout << "Execute Data\n";
		    OCL_CHECK(err, err = kESP.setArg(7,i_step));
		    OCL_CHECK(err, err = kESP.setArg(8,i_run));
			OCL_CHECK(err, err = q[0].enqueueMigrateMemObjects({b_obs,b_pxxIn,b_stateIn,b_wtIn},0/* 0 means from host*/));
			//Launch the Kernel
			OCL_CHECK(err, err = q[0].enqueueTask(kESP));
			OCL_CHECK(err, err = q[0].enqueueMigrateMemObjects({b_pxxOut,b_stateOut,b_wtOut},CL_MIGRATE_MEM_OBJECT_HOST));
			q[0].finish();

			memcpy(stateOut,p_stateOut,size_state);
			memcpy(pxxOut,p_pxxOut,size_pxx);
//			range.end();
			cout << "Iteration " << i_step << ": " << (double)stateOut[0] <<  ", "
					<< (double)stateOut[1] << " Pxx= " << pxxOut[0] << ", " << pxxOut[14] << "\n";
			write_csv("/mnt/result/state_sol.csv",convert_double(stateOut,1,NUM_VAR,-1),1,NUM_VAR);
			write_csv("/mnt/result/pxx_sol.csv",convert_double(pxxOut,1,NUM_VAR*NUM_VAR,-1),NUM_VAR,NUM_VAR);
		}
    }



    // play with the interrupt
    cout << "\nDone. Good Luck with the Result, My Lord \n";

//    OCL_CHECK(err, err = q[0].finish());
//    OCL_CHECK(err, err = q[1].finish());

    int match =0;

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);

}

