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

//#include "experimental/xrt_profile.h"
//#include "xcl2.hpp"
//#include "experimental/xrt_profile.h"

#include "vadd.h"
#include "global_define/global_define.h"
#include "global_define/read_write_csv.h"
#include "global_define/mat_lib.h"
#include "random_generator/normrnd.h"
#include "random_generator/randn.h"
#include "resample_pf/resample_pf.h"
#include "mvnpdf/mvnpdf_code.h"
#include "calweights/Calweights.h"
#include "rk4/rk4.h"
#include "global_define/GISmsmt_prcs.h"

//static const std::string error_message =
//    "Error: Result mismatch:\n"
//    "i = %d CPU result = %d Device result = %d\n";
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
    OCL_CHECK(err, err = kSigma.setArg(0,b_pxxIn));
    OCL_CHECK(err, err = kSigma.setArg(1,b_sigMatOut));
    OCL_CHECK(err, err = kSigma.setArg(2,b_rndIn));

    OCL_CHECK(err, err = kCreate.setArg(0,b_stateIn));
    OCL_CHECK(err, err = kCreate.setArg(1,b_sigMatIn));
    OCL_CHECK(err, err = kCreate.setArg(2,b_prtclsOut));

    OCL_CHECK(err, err = k_mPxx.setArg(0,b_prtclsIn));
    OCL_CHECK(err, err = k_mPxx.setArg(1,b_pxxOut));

    OCL_CHECK(err, err = kCal.setArg(0,b_prtclsIn));
    OCL_CHECK(err, err = kCal.setArg(1,b_msmtIn));
    OCL_CHECK(err, err = kCal.setArg(2,b_RmatIn));
    OCL_CHECK(err, err = kCal.setArg(3,0));
    OCL_CHECK(err, err = kCal.setArg(4,b_pxxIn));
    OCL_CHECK(err, err = kCal.setArg(5,b_zDiffOut));
    OCL_CHECK(err, err = kCal.setArg(6,b_pzxOut));

    OCL_CHECK(err, err = kPFU.setArg(0,b_prtclsIn));
    OCL_CHECK(err, err = kPFU.setArg(1,b_wtIn));
    OCL_CHECK(err, err = kPFU.setArg(2,b_stateOut));
    OCL_CHECK(err, err = kPFU.setArg(3,b_pxxOut));

    //We then need to map our OpenCL buffers to get the pointers
    int *p_rndIn,*p_stateIn,*p_sigMatIn,*p_prtclsIn,*p_msmtIn,*p_RmatIn,*p_pxxIn,*p_wtIn,
		*p_sigMatOut,*p_prtclsOut,*p_zDiff,*p_pzx,*p_stateOut,*p_pxxOut;
    cout << "Phase: Link Pointer\n";


	// link input pointer
	buf_link(&p_rndIn,b_rndIn,size_large,WBUF,1);
	buf_link(&p_stateIn,b_stateIn,size_state,WBUF,1);
	buf_link(&p_sigMatIn,b_sigMatIn,size_large,WBUF,0);
	buf_link(&p_prtclsIn,b_prtclsIn,size_large,WBUF,1);
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
    rand_init(0);
    for(int i_run = 0; i_run < 1;i_run++)
    {
        int step =0;

	//  range.end();
	//	events.mark("Initializes Variables and Data Test");
        fixed_type prtcls[NUM_VAR*NUM_PARTICLES];
        fixed_type state[NUM_VAR];
        fixed_type pxx[NUM_VAR*NUM_VAR];
        fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES];
        fixed_type rnd_rk4[NUM_VAR*4];
        fixed_type sigMat[NUM_VAR*NUM_PARTICLES];

        fixed_type wt[1*NUM_PARTICLES];
        double N_eff =0;
        fixed_type obs_data[52*10];
        convert_FP(read_csvMulLine("/mnt/test_data/Init/obsVals.csv",0, 52, 10),
        		obs_data, 1, 52*10, -1);
        Mat_S obs;
        init_mat(&obs,1,10);
		for(int i_step=0; i_step < 52;i_step++)
		{
			for(int i=0; i < 10;i++)
			{
				obs.entries[i] = obs_data[i_step*10+i];
			}
			if(i_step==0)
			{
		        cout << "Phase: initialisation \n";
		        // only read the first one
		        convert_FP(read_csvMulLine("/mnt/test_data/Init/prtcls_init.csv",0, NUM_VAR, NUM_PARTICLES),
											prtcls, NUM_VAR, NUM_PARTICLES, 1);
		        convert_FP(read_csvMulLine("/mnt/test_data/Init/state_init.csv",0*NUM_VAR, NUM_VAR, 1),
											state, 1, NUM_VAR, -1);
		        convert_FP(read_csvMulLine("/mnt/test_data/Init/Pxx_in.csv",0*NUM_VAR, NUM_VAR, NUM_VAR),
											pxx, NUM_VAR, NUM_VAR, 0);
		        for(int i=0; i < NUM_PARTICLES;i++)
		        {
		        	wt[i] = 1.0/NUM_PARTICLES;
		        }
		        memcpy(p_stateIn,state,size_state);
		        memcpy(p_prtclsIn,prtcls,size_large);
		        memcpy(p_pxxIn,pxx,size_pxx);
			}
			else
			{
				memcpy(p_stateIn,p_stateOut,size_state);
				memcpy(p_pxxIn,p_pxxOut,size_pxx);
				memcpy(state,p_stateOut,size_state);
				memcpy(pxx,p_pxxOut,size_pxx);


			}
			for(int i=0; i< NUM_VAR*NUM_PARTICLES;i++)
			{
				double rnd_temp;
				randn(&rnd_temp,0,0);
				rnd_sigma[i] = rnd_temp;
			}

			for(int i=0; i< NUM_VAR*4;i++)
			{
				double rnd_temp;
				randn(&rnd_temp,0,0);
				rnd_rk4[i] = rnd_temp;
			}
			block_S(&p_pxxIn,pxx, b_pxxIn,
					&p_stateIn,state,b_stateIn,rnd_rk4,
					&p_sigMatIn,sigMat,b_sigMatIn,
					&p_sigMatOut, b_sigMatOut,
					&p_rndIn,rnd_sigma,b_rndIn,
					&p_prtclsOut, prtcls,b_prtclsOut,
					kSigma,kCreate,0);

			block_C(&p_prtclsOut,&p_prtclsIn,b_prtclsIn,
					&p_pxxOut,b_pxxOut,
					&obs,
					&p_msmtIn,b_msmtIn,
					&p_RmatIn, b_RmatIn,
					&p_pxxIn, b_pxxIn,
					&p_zDiff, b_zDiffOut,
					&p_pzx,b_pzxOut,
					i_step, &N_eff,
					wt,
					k_mPxx,kCal,
					0);

			block_R(prtcls,wt, N_eff,
					&p_prtclsIn,b_prtclsIn,
					&p_wtIn,b_wtIn,
					&p_stateOut, b_stateOut,
					&p_pxxOut, b_pxxOut,
					&p_stateIn, &p_pxxIn,
					step,
					kPFU,0);
		}
    }

    cout << "\nDone. Good Luck with the Result, My Lord \n";

    OCL_CHECK(err, err = q[0].finish());
    OCL_CHECK(err, err = q[1].finish());

    int match =0;

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl; 
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);

}

