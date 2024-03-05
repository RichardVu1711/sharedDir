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

// TODO: NEED TO WRITE A FUNCTION TO CONVERT FROM PARTICLE PORTION TO THE ORIGINAL ALLIGNMENT
// TODO: SIZE OF FIFO SHOULD MATCH WITH SIZE OF BUFFER => reduce hw consumption
int main(int argc, char** argv) {
    // clean the house
    system(" rm -rf /mnt/result/*.csv");
	ESP_PF imp(&argc,&argv);
	vector<string> datapaths = {"/mnt/test_data/obsVal1/Init/obsVal1.csv",
								"/mnt/test_data/obsVal2/Init/obsVal2.csv"};
	srcObj srcx [N_SRC];
	cl_int err;
	rndCtrl rng;

	// load data and use the kernel_exec function
	imp.releaseBuff();

    bool match = true;
    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
