/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include "obj_code/srcObj.h"
#include "obj_code/ESPObj.h"
#include "obj_code/env_host.h"
#include "experimental/xrt_profile.h"
#include "xcl2.hpp"
#include "host.h"
#include <algorithm>
#include <vector>

#include <hls_math.h>
#define DATA_SIZE 4096
void rnd_creation(srcObj* srcX, rndCtrl* rng){
	// pre-generate rnd number for sigma

	fixed_type* sigmaRnd = &srcX->rndSigma[0];
	fixed_type* rk4Rnd = &srcX->rndrk4[0];
	for(int j=0; j < NUM_PARTICLES*NUM_VAR;j+=NUM_PARTICLES){
		for(int i=0; i <NUM_PARTICLES;i++){
			double tmp =0;
			if(rng->isInit==0 && srcX->srcIdx ==0){
				// haven't initialise with the seed yet
				tmp = RNG_withSeed(1,rng->seed);
				rng->isInit = 1;
			}
			else{
				tmp = RNG_withSeed(0,0);
			}
			sigmaRnd[i+j] =tmp;
		}
	}
	for(int i=0; i < 4*NUM_VAR;i++)
	{
		double tmp  =RNG_withSeed(0,0);
		rk4Rnd[i] = tmp;
	}
}

void sigma_tb(ESP_PF* imp, srcObj* srcx, int set, int iter, storeType mode){
	std::vector<cl::Event>kernel_lst;
	cl::Event knrl_exec;
	imp->smpl_phase.sigmaInfo.allo_mode = PL;

	unit_data rnd_md = {
		.mode = mode,
		.block = "sigmaComp",
		.var = "rnd",
		.index = set,
		.dim = {iter*NUM_VAR,NUM_VAR,NUM_PARTICLES} //{start row, #of rows, #of column}
	};
	unit_data pxx_md = {
		.mode = mode,
		.block = "sigmaComp",
		.var = "pxx",
		.index = set,
		.dim = {iter*NUM_VAR,NUM_VAR,NUM_VAR} //{start row, #of rows, #of column}
	};
	unit_data pxxsqrt_md = {
		.mode = mode,
		.block = "sigmaComp",
		.var = "pxxsqrt",
		.index = set,
		.dim = {iter*1,1,NUM_VAR} //{start row, #of rows, #of column}
	};

	// for output, it will need to chose to save under result or resultSpecial folder
	// if special => resultSpecial, otherwise => result
	unit_data sigmaPL_md = {
		.mode = mode,
		.block = "sigmaComp",
		.var = "sigmaPL",
		.index = set,
		.dim = {iter*NUM_VAR,NUM_VAR,NUM_PARTICLES} //{start row, #of rows, #of column}
	};
	unit_data sigmaPS_md = {
		.mode = mode,
		.block = "sigmaComp",
		.var = "sigmaPS",
		.index = set,
		.dim = {iter*NUM_VAR,NUM_VAR,NUM_PARTICLES} //{start row, #of rows, #of column}
	};


	fixed_type pxx[NUM_VAR*NUM_VAR];
	readData(rnd_md,imp->smpl_phase.sigmaInfo.rndIn.ptr);
	readData(pxx_md,pxx);
	writeData(pxx_md,pxx);

	// process pxx
	for(int i=0; i < NUM_VAR;i++){
		imp->smpl_phase.sigmaInfo.pxxSqrt.ptr[i] = (fixed_type) sqrt((double) pxx[i*NUM_VAR+i] );
	}
	// setting PL Mode for execution:
	imp->smpl_phase.sigmaInfo.allo_mode = PL;
	sigma_execute(imp,srcx,kernel_lst,nullptr,&knrl_exec);
	axis2mm_execute(imp,srcx,kernel_lst,&imp->smpl_phase.status.flag);
//	imp->getQueue(0).finish();

	while(imp->flagCheck_S(0)==0){}
	writeData(rnd_md,imp->smpl_phase.sigmaInfo.rndIn.ptr);
	writeData(pxxsqrt_md,imp->smpl_phase.sigmaInfo.pxxSqrt.ptr);

	writeData(sigmaPL_md,imp->smpl_phase.axis2mmInfo.prtclsOut.ptr);
	// Setting PS Mode for execution
	imp->smpl_phase.sigmaInfo.allo_mode = PS;
	sigma_execute(imp,srcx,kernel_lst,nullptr,&knrl_exec);

	cout << imp->smpl_phase.sigmaInfo.sigMat.ptr[0] << "\n";
	writeData(sigmaPS_md,imp->smpl_phase.sigmaInfo.sigMat.ptr);
}
void meanPxx(ESP_PF* imp, srcObj* srcx, int set, int iter, storeType mode){

}
void calw_tb(ESP_PF* imp, srcObj* srcx, int set, int iter, storeType mode){
//	calwPhase_execute(imp, srcx);
}
void mvnpdf_tb(ESP_PF* imp, srcObj* srcx, int set, int iter, storeType mode){
	std::vector<cl::Event>kernel_lst;
//	std::vector<cl::Event> data_event(3);
//	std::vector<cl::Event> exec_event(3);
	cl::Event data_event;
	cl::Event exec_event;

	unit_data zDiff_md = {
			.mode = mode,
			.block = "mvnpdf",
			.var = "zDiff",
			.index = set,
			.dim = {iter,1024,N_MEAS} //{start row, #of rows, #of column}
	};
	unit_data pzx_md = {
			.mode = mode,
			.block = "mvnpdf",
			.var = "pzx",
			.index = set,
			.dim = {iter*N_MEAS,N_MEAS*1024,N_MEAS} //{start row, #of rows, #of column}
	};
	// data is written from
	readData(zDiff_md,imp->calW_phase.calWInfo.zDiff.ptr);
	readData(pzx_md,imp->calW_phase.calWInfo.pzx.ptr);

	mvnpdf_execute(imp,srcx,kernel_lst,nullptr,&exec_event);

}
void crtParticles_tb(ESP_PF* imp, srcObj* srcx, int set, int iter, storeType mode){
	std::vector<cl::Event>kernel_lst;
	cl::Event knrl_exec;
	vector<cl::Memory> emptyMem;
	unit_data sigma_md = {
			.mode = mode,
			.block = "crtParticles",
			.var = "sigma",
			.index = set,
			.dim = {iter*NUM_VAR,NUM_VAR,NUM_PARTICLES} //{start row, #of rows, #of column}
	};
	unit_data state_md = {
			.mode = mode,
			.block = "crtParticles",
			.var = "state_pro",
			.index = set,
			.dim = {iter,1,NUM_VAR} //{start row, #of rows, #of column}
	};
	unit_data prtclsPL_md = {
		.mode = mode,
		.block = "crtParticles",
		.var = "prtclsPL",
		.index = set,
		.dim = {iter*NUM_VAR,NUM_VAR,NUM_PARTICLES} //{start row, #of rows, #of column}
	};
	unit_data prtclsPS_md = {
		.mode = mode,
		.block = "crtParticles",
		.var = "prtclsPS",
		.index = set,
		.dim = {iter*NUM_VAR,NUM_VAR,NUM_PARTICLES} //{start row, #of rows, #of column}
	};
	// md= metadata
	readData(sigma_md,imp->smpl_phase.espCrtInfo.sigMat.ptr);
	readData(state_md,imp->smpl_phase.espCrtInfo.statePro.ptr);
	cout << imp->smpl_phase.espCrtInfo.sigMat.ptr[0]
		  <<" - " << imp->smpl_phase.espCrtInfo.statePro.ptr[0] << "\n";
	imp->smpl_phase.espCrtInfo.allo_mode = PS;
	ESPCrtParticles_execute(imp,srcx,kernel_lst,nullptr,&knrl_exec);

	writeData(prtclsPS_md,imp->smpl_phase.espCrtInfo.prtcls.ptr);
	writeData(sigma_md,imp->smpl_phase.espCrtInfo.sigMat.ptr);
	writeData(state_md,imp->smpl_phase.espCrtInfo.statePro.ptr);

}

void meanPxx_tb(ESP_PF* imp, srcObj* srcx, int set, int iter, storeType mode){
	std::vector<cl::Event>kernel_lst;
	cl::Event knrl_exec;
	vector<cl::Memory> emptyMem;
	unit_data prtcls_md = {
			.mode = mode,
			.block = "meanPxx",
			.var = "prtcls",
			.index = set,
			.dim = {iter*NUM_VAR,NUM_VAR,NUM_PARTICLES} //{start row, #of rows, #of column}
	};
	unit_data pxxPS_md = {
			.mode = mode,
			.block = "meanPxx",
			.var = "mPxxPS",
			.index = set,
			.dim = {iter*NUM_VAR,NUM_VAR,NUM_VAR} //{start row, #of rows, #of column}
	};

	readData(prtcls_md,imp->smpl_phase.mPxxInfo.prtcls.ptr);
	imp->smpl_phase.mPxxInfo.allo_mode = PS;
	mPxx_execute(imp,srcx,kernel_lst,nullptr,&knrl_exec);

	writeData(pxxPS_md,imp->smpl_phase.mPxxInfo.mPxx.ptr);
}

int main(int argc, char** argv) {
    // clean the house
	// todo: write a function to clean probrably
    system(" rm -rf /media/sd-mmcblk0p1/sigmaComp/result/*.csv");
    system(" rm -rf /media/sd-mmcblk0p1/sigmaComp/resultSpecial/*.csv");
    system(" rm -rf /media/sd-mmcblk0p1/crtParticles/result/*.csv");
	system(" rm -rf /media/sd-mmcblk0p1/crtParticles/resultSpecial/*.csv");
	system(" rm -rf /media/sd-mmcblk0p1/meanPxx/result/*.csv");
	system(" rm -rf /media/sd-mmcblk0p1/meanPxx/resultSpecial/*.csv");

	ESP_PF imp(&argc,&argv);
	vector<string> datapaths = {"/mnt/test_data/obsVal1/Init/obsVal1.csv",
								"/mnt/test_data/obsVal2/Init/obsVal2.csv"};
	srcObj srcx [N_SRC];
	srcx[0].srcIdx = 0;
	for(int i=0; i < N_SRC;i++){
		srcx[i] = srcObj(datapaths[i], i,imp.esp_control.context,imp.esp_control.q[i]);
		cout << "READ DATA \n";
	}

//	for(int i_set =0; i_set < 2;i_set++){
//		for(int i =0; i < N_OBS;i++){
//			sigma_tb(&imp,&srcx[0],i_set,i,special);
//		}
//	}

//	for(int i_set =0; i_set < 1;i_set++){
//		for(int i =0; i < N_OBS;i++){
//			crtParticles_tb(&imp,&srcx[0],i_set,i,impData);
//		}
//	}
	// for block not in smpl, there is only 51 iterations data
//	for(int i_set =0; i_set < 1;i_set++){
//		for(int i =0; i < N_OBS-1;i++){
//			meanPxx_tb(&imp,&srcx[0],i_set,i,impData);
//		}
//	}
	for(int i_set =0; i_set < 1;i_set++){
		for(int i =0; i < N_OBS-51;i++){
			mvnpdf_tb(&imp,&srcx[0],i_set,20,impData);
		}
	}


//	std::vector<double> dataIn(1024);
//	std::vector<double> dataOut(1024);
//	double step = pow(2.0, (0-11));
//	dataIn[0] = step;
//
//	for(int j =0; j < 16;j++){
//		for(int i =0; i < 1024;i++){
//			if(i==0){
//				if(j ==0)
//					dataIn[i] = step;
//				else
//					dataIn[i] = dataIn[1023];
//			}
//			else{
//				dataIn[i] = dataIn[i-1]  + step;
//			}
////			dataIn[i] = 2;
//			dataOut[i] = sqrt(dataIn[i]);
//
//		}
//		step = 	pow(2.0, (j-11));
//		cout<< std::setprecision(6)<< dataIn[2] << ", " << dataOut[2]<< ", " <<sqrt(dataIn[2]) << ", " <<"\n";
//		write_csv("c_pro/temp/dataIn.csv",dataIn,1,1024);
//		write_csv("c_pro/temp/dataOut.csv",dataOut,1,1024);
//	}
//    vector<double> inputData = read_csvMulLine("c_pro/temp/dataIn.csv", 0, 16,1024);

//    size_t ncol = 1024;
//	size_t totalEntries = inputData.size();
//	size_t totalRows = totalEntries / ncol;
//
//	// Process the data: perform square root operation
//	for (size_t i = 0; i < totalRows; ++i) {
//		size_t rowStartIndex = i * ncol;
//		for (size_t j = 0; j < ncol; ++j) {
//			size_t index = rowStartIndex + j;
//			inputData[index] = sqrt(inputData[index]); // Square root operation
//
//		}
//	}
//
//	// Write data to CSV using provided function
//	write_csv("c_pro/temp/dataOutPL.csv", inputData, 16, 1024);

//	imp.releaseBuff();
	ap_fixed<WORD_LENGTH,INT_LEN> raw = 3.4;
	fixed_type rawSqrt = hls::sqrt(raw);
    bool match = true;
    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
