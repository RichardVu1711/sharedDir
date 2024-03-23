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

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include "vadd.h"
#include "global_define/global_define.h"
#include "global_define/mat_lib.h"
#include "global_define/read_write_csv.h"
#include "random_generator/normrnd.h"
#include "GISmsmt_prcs.h"

static const std::string error_message =
    "Error: Result mismatch:\n"
    "i = %d CPU result = %d Device result = %d\n";

int main(int argc, char* argv[]) {

    //TARGET_DEVICE macro needs to be passed from gcc command line
    if(argc != 2) {
		std::cout << "Usage: " << argv[0] <<" <xclbin>" << std::endl;
		return EXIT_FAILURE;
	}

    std::string xclbinFilename = argv[1];
    
    // Compute the size of array in bytes
    size_t size_Mat_S = 1 * sizeof(Mat_S);
    size_t size_Mat = 1 * sizeof(Mat);
    size_t size_wt = NUM_PARTICLES*sizeof(fixed_type);
    // Creates a vector of DATA_SIZE elements with an initial value of 10 and 32
    // using customized allocator for getting buffer alignment to 4k boundary
    
    std::vector<cl::Device> devices;
    cl::Device device;
    cl_int err;
    cl::Context context;
    cl::CommandQueue q;
    cl::Kernel krnl_vector_add;
    cl::Program program;
    std::vector<cl::Platform> platforms;
    bool found_device = false;

    //traversing all Platforms To find Xilinx Platform and targeted
    //Device in Xilinx Platform
    cl::Platform::get(&platforms);
    for(size_t i = 0; (i < platforms.size() ) & (found_device == false) ;i++){
        cl::Platform platform = platforms[i];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
        if ( platformName == "Xilinx"){
            devices.clear();
            platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
	    if (devices.size()){
		    device = devices[0];
		    found_device = true;
		    break;
	    }
        }
    }
    if (found_device == false){
       std::cout << "Error: Unable to find Target Device " 
           << device.getInfo<CL_DEVICE_NAME>() << std::endl;
       return EXIT_FAILURE; 
    }

    // Creating Context and Command Queue for selected device
    OCL_CHECK(err, context = cl::Context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

    std::cout << "INFO: Reading " << xclbinFilename << std::endl;
    FILE* fp;
    if ((fp = fopen(xclbinFilename.c_str(), "r")) == nullptr) {
        printf("ERROR: %s xclbin not available please build\n", xclbinFilename.c_str());
        exit(EXIT_FAILURE);
    }
    // Load xclbin 
    std::cout << "Loading: '" << xclbinFilename << "'\n";
    std::ifstream bin_file(xclbinFilename, std::ifstream::binary);
    bin_file.seekg (0, bin_file.end);
    unsigned nb = bin_file.tellg();
    bin_file.seekg (0, bin_file.beg);
    char *buf = new char [nb];
    bin_file.read(buf, nb);
    
    // Creating Program from Binary File
    cl::Program::Binaries bins;
    bins.push_back({buf,nb});
    devices.resize(1);
    OCL_CHECK(err, program = cl::Program(context, devices, bins, NULL, &err));
    
    // This call will get the kernel object from program. A kernel is an 
    // OpenCL function that is executed on the FPGA. 
    OCL_CHECK(err, cl::Kernel krnl_ESP_PF = cl::Kernel(program,"ESP_PF_Wrapper", &err));
    // These commands will allocate memory on the Device. The cl::Buffer objects can
    // be used to reference the memory locations on the device. 

    fixed_type obs_data[53*10] = {	210.2,348,127.2,-33.1582593,-10.56135711,22.60725518,-115.7937302,-200.6948855,-115.0369554,-199.0540967,
									210.1,348,127.3,-33.14031648,-10.56269584,22.60425274,-115.7248797,-201.0936181,-115.0395667,-199.0510516,
									210.1,348,127.3,-33.15137164,-10.54746021,22.60110522,-115.781628,-201.0941906,-115.0425481,-199.0509848,
									210.1,348,127.2,-33.12527522,-10.53463015,22.61238046,-115.8054479,-200.8931397,-115.0404741,-199.0482954,
									210.1,348,127.2,-33.11734407,-10.54277999,22.58954313,-115.7674489,-200.9291873,-115.0427345,-199.0514421,
									210.1,348,127.1,-33.14129046,-10.52456539,22.65312975,-115.8398447,-200.5925435,-115.03917,-199.0475503,
									210.2,348,127.1,-33.1770565,-10.53443609,22.67391479,-115.8966324,-200.3678026,-115.0403961,-199.0474645,
									210.1,348,127.1,-33.18019682,-10.51902645,22.67713468,-115.8777338,-200.5104783,-115.0429438,-199.0461676,
									210.1,348,127.2,-33.21249734,-10.52718162,22.72541388,-115.8896598,-200.6242597,-115.0408369,-199.0439227,
									210.1,348.1,127.2,-33.22270547,-10.51045401,22.76296518,-115.6496485,-200.5108331,-115.0382452,-199.0487367,
									210.1,348,127.1,-33.21664557,-10.52955137,22.74853929,-115.8711537,-200.3547437,-115.0443343,-199.048498,
									210.2,348,127.3,-33.25240427,-10.50914183,22.81197515,-116.0840419,-200.554973,-115.043666,-199.0479775,
									210.1,348,127.3,-33.25510852,-10.50883281,22.80054926,-116.0067476,-200.7074445,-115.0412137,-199.0518354,
									210.1,348.1,127.3,-33.22254364,-10.44376935,22.84235225,-115.9281298,-200.6832506,-115.044824,-199.0533193,
									210.1,348,127.3,-33.24028806,-10.42818524,22.89722674,-116.3176437,-200.6552435,-115.0415763,-199.0513989,
									210.1,348,127.3,-33.2121938,-10.39698822,22.88930489,-116.4185731,-200.7279251,-115.0443673,-199.0498872,
									210.1,348,127.3,-33.18296693,-10.35630303,22.91383331,-116.5622346,-200.7646578,-115.0387591,-199.0483618,
									210.1,348,127.3,-33.18856701,-10.34597378,22.90507866,-116.5981252,-200.7766932,-115.040154,-199.051741,
									210.1,348,127.3,-33.16044842,-10.33637357,22.90500927,-116.6244005,-200.8225317,-115.0439748,-199.0483258,
									210.1,348,127.3,-33.14850852,-10.3592732,22.85182896,-116.5243853,-200.8948794,-115.0482747,-199.047647,
									210.1,347.9,127.2,-33.17148413,-10.38385548,22.85846326,-116.7410679,-200.6639779,-115.0427213,-199.048767,
									210.1,347.9,127.2,-33.1637083,-10.40640546,22.82331017,-116.6484682,-200.7060828,-115.041697,-199.0501807,
									210.1,348,127.2,-33.17376274,-10.41467841,22.80562497,-116.3030169,-200.6513349,-115.0453105,-199.0560246,
									210.1,348,127.1,-33.17855464,-10.42353787,22.81388517,-116.2572533,-200.396482,-115.0413342,-199.0491842,
									210.1,348,127.2,-33.18165067,-10.45681205,22.79097381,-116.1507908,-200.6291876,-115.0422808,-199.0474108,
									210.1,348,127.2,-33.18256661,-10.46586313,22.74647061,-116.1060034,-200.6822091,-115.0485922,-199.0536285,
									210.1,348,127.3,-33.16397252,-10.49020918,22.72760405,-116.0261773,-200.9463682,-115.046081,-199.0497662,
									210.1,348,127.2,-33.19098525,-10.49750231,22.7305481,-115.9909471,-200.6688908,-115.0473027,-199.0541612,
									210.1,348,127.3,-33.17932184,-10.51359746,22.71092508,-115.942258,-200.9307237,-115.0435914,-199.049734,
									210.1,348,127.2,-33.14904056,-10.55297254,22.59174753,-115.7407802,-200.875138,-115.0449993,-199.0523519,
									210.1,348,127.2,-33.14848278,-10.54466478,22.6256827,-115.7800871,-200.8356025,-115.0395131,-199.0492623,
									210.1,348,127.3,-33.13612353,-10.54195289,22.60917616,-115.7992767,-201.1081591,-115.0396984,-199.0490142,
									210.1,348,127.2,-33.13904368,-10.52180823,22.61879229,-115.8570914,-200.8750809,-115.0437767,-199.0471619,
									210.1,348,127.2,-33.13848598,-10.52718802,22.61777478,-115.8374199,-200.8732235,-115.0435882,-199.0481789,
									210.1,348,127.2,-33.12767171,-10.52366533,22.61808726,-115.8469706,-200.8902326,-115.040713,-199.0467113,
									210.1,348,127.1,-33.13558368,-10.53324438,22.60386661,-115.7930194,-200.6613665,-115.0422142,-199.0526336,
									210.1,348,127.2,-33.13817673,-10.52705075,22.62109957,-115.83875,-200.8691867,-115.044915,-199.0506547,
									210.1,348,127.2,-33.16047065,-10.53859359,22.64722525,-115.8114482,-200.794221,-115.0453368,-199.0527199,
									210.1,348,127.2,-33.14694257,-10.54220551,22.631625,-115.7901178,-200.8314192,-115.043722,-199.04627,
									210.1,348,127.1,-33.13981687,-10.53653664,22.64069283,-115.7930565,-200.6026753,-115.0462235,-199.0503651,
									210.1,348,127.2,-33.1394826,-10.53938677,22.64320001,-115.8014072,-200.8278928,-115.0438479,-199.0478668,
									210.1,348,127.1,-33.11669233,-10.52987074,22.59492414,-115.7970824,-200.7020286,-115.0424934,-199.051295,
									210.1,348,127.4,-33.0921999,-10.54911367,22.53873901,-115.7583414,-201.4910417,-115.0458059,-199.0507301,
									210.1,348,127.2,-33.08596671,-10.53837936,22.56769598,-115.7678272,-201.0053832,-115.0467232,-199.0538222,
									210.1,348,127.3,-33.0758024,-10.53343993,22.55965489,-115.7980677,-201.2650054,-115.0438456,-199.0481486,
									210.1,348,127.3,-33.08097392,-10.53638122,22.55643102,-115.7881102,-201.2601306,-115.0423062,-199.050764,
									210.1,348,127.3,-33.04196981,-10.5289499,22.54284639,-115.7995461,-201.3377137,-115.038514,-199.049551,
									210.1,348,127.3,-33.04401604,-10.50582193,22.57141036,-115.8911221,-201.3131371,-115.0387758,-199.0554579,
									210.1,348,127.3,-33.05323909,-10.48687159,22.58169749,-115.9645511,-201.3005931,-115.0409716,-199.0473767,
									210.1,348,127.3,-33.05615145,-10.48300022,22.57975828,-115.9786416,-201.3021535,-115.041606,-199.0502008,
									210.1,348,127.3,-33.06656804,-10.48907592,22.59475235,-115.9641924,-201.2627907,-115.0379169,-199.0512581,
									210.1,348,127.3,-33.07257362,-10.5014856,22.59266655,-115.9209624,-201.2481374,-115.0448057,-199.0499128,
									210.1,348,127.2,-33.09560751,-10.49561686,22.61559092,-115.9373455,-200.9585866,-115.0414998,-199.0535784
    };

    fixed_type pxx_data[NUM_VAR*NUM_VAR] = {10,0,0,0,0,0,0,0,0,0,0,0,0,
											0,10,0,0,0,0,0,0,0,0,0,0,0,
											0,0,1,0,0,0,0,0,0,0,0,0,0,
											0,0,0,1,0,0,0,0,0,0,0,0,0,
											0,0,0,0,0.01,0,0,0,0,0,0,0,0,
											0,0,0,0,0,0.01,0,0,0,0,0,0,0,
											0,0,0,0,0,0,0.01,0,0,0,0,0,0,
											0,0,0,0,0,0,0,0.01,0,0,0,0,0,
											0,0,0,0,0,0,0,0,0.01,0,0,0,0,
											0,0,0,0,0,0,0,0,0,0.01,0,0,0,
											0,0,0,0,0,0,0,0,0,0,0.001,0,0,
											0,0,0,0,0,0,0,0,0,0,0,0.001,0,
											0,0,0,0,0,0,0,0,0,0,0,0,0.001};

    fixed_type state_data[NUM_VAR] = {-114.94,-198.95,0,0,0,0,0,0,0,0,0,0,0};
    // declear input and output matrix
    Mat_S obs;
    init_mat(&obs,1,10);


    Mat_S pxxIn;
    init_mat(&pxxIn,NUM_VAR, NUM_VAR);

    Mat_S stateIn;
    init_mat(&stateIn,NUM_VAR,1);

    fixed_type wtIn[NUM_PARTICLES];

    Mat prtclsIn,data_out;
    newmat(&prtclsIn,NUM_VAR,NUM_PARTICLES);
    newmat(&data_out,13,52);

    Mat_S stateOut,pxxOut;
    Mat prtclsOut;
    fixed_type wtOut[NUM_PARTICLES];

    // input buffers initialized
    OCL_CHECK(err, cl::Buffer buffer_obsFP(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_pxxFP(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_stateFP(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_wt(context, CL_MEM_READ_ONLY, size_wt, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_prtcls(context, CL_MEM_READ_ONLY, size_Mat, NULL, &err));

    // output buffers initialized
    OCL_CHECK(err, cl::Buffer buffer_stateOut(context, CL_MEM_WRITE_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_pxxOut(context, CL_MEM_WRITE_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_prtclsOut(context, CL_MEM_WRITE_ONLY, size_Mat, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_wtOut(context, CL_MEM_WRITE_ONLY, size_wt, NULL, &err));
    int step =0;

    //set the kernel Arguments
    int narg=0;
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_obsFP));
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_pxxFP));
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_stateFP));
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_wt));
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_prtcls));

    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_stateOut));
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_pxxOut));
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_prtclsOut));
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,buffer_wtOut));
    OCL_CHECK(err, err = krnl_ESP_PF.setArg(narg++,step));

    //We then need to map our OpenCL buffers to get the pointers
    int *ptr_obs,*ptr_pxxIn,*prt_stateIn,*ptr_wtIn,*ptr_prtcls,
		*ptr_stateOut, *ptr_pxxOut,*ptr_prtclsOut,*ptr_wtOut;

    OCL_CHECK(err, ptr_obs = (int*)q.enqueueMapBuffer (buffer_obsFP , CL_TRUE , CL_MAP_WRITE , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_pxxIn = (int*)q.enqueueMapBuffer (buffer_pxxFP , CL_TRUE , CL_MAP_WRITE , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, prt_stateIn = (int*)q.enqueueMapBuffer (buffer_stateFP , CL_TRUE , CL_MAP_WRITE , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_wtIn = (int*)q.enqueueMapBuffer (buffer_wt , CL_TRUE , CL_MAP_WRITE , 0, size_wt, NULL, NULL, &err));
    OCL_CHECK(err, ptr_prtcls = (int*)q.enqueueMapBuffer (buffer_prtcls , CL_TRUE , CL_MAP_WRITE , 0, size_Mat, NULL, NULL, &err));

    OCL_CHECK(err, ptr_stateOut = (int*)q.enqueueMapBuffer (buffer_stateOut , CL_TRUE , CL_MAP_READ , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_pxxOut = (int*)q.enqueueMapBuffer (buffer_pxxOut , CL_TRUE , CL_MAP_READ , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_prtclsOut = (int*)q.enqueueMapBuffer (buffer_prtclsOut , CL_TRUE , CL_MAP_READ , 0, size_Mat, NULL, NULL, &err));
    OCL_CHECK(err, ptr_wtOut = (int*)q.enqueueMapBuffer (buffer_wtOut , CL_TRUE , CL_MAP_READ , 0, size_wt, NULL, NULL, &err));
    for(int i0 = 0; i0 < 1;i0++)
    {
		int num_loop = 52;
		int step =0;
		for(int k=0; k < num_loop;k++)
		{
			for(int i =0; i < 10;i ++)
			{
				obs.entries[i] = obs_data[step*10 + i];
			}
			memcpy(ptr_obs,&obs,size_Mat_S);
	//		showmat_S(&obs);
	//	    msmt msmtinfo =msmt_prcs(&obs);
	//	    showmat_S(&(msmtinfo.z));
			if(step==0)
			{
				for(int i =0; i< NUM_PARTICLES;i++)
				{
					wtIn[i] = 1.0/NUM_PARTICLES;
				}
				for(int i =0; i<NUM_VAR*NUM_VAR;i++)
				{
					pxxIn.entries[i] = pxx_data[i];
				}
				for(int i =0; i< NUM_VAR;i++)
				{
					stateIn.entries[i*NUM_VAR] = state_data[i];
				}
				for (int i = 0; i < NUM_VAR; i++)
				{
					ap_fixed<WORD_LENGTH,INT_LEN> temp = get_ele_S(&pxxIn,i,i);

					for(int j=0; j < NUM_PARTICLES;j++)
					{
						fixed_type r = normrnd(0,hls::sqrt(temp));
						fixed_type temp_fp = r;
						set_ele(&prtclsIn,i,j,stateIn.entries[i*13]+temp_fp);
					}
				}
				memcpy(ptr_pxxIn,&pxxIn,size_Mat_S);
				memcpy(prt_stateIn,&stateIn,size_Mat_S);
				memcpy(ptr_wtIn,&wtIn,size_wt);
				memcpy(ptr_prtcls,&prtclsIn,size_Mat);
			}
			else
			{
				memcpy(ptr_pxxIn,ptr_pxxOut,size_Mat_S);
				memcpy(prt_stateIn,ptr_stateOut,size_Mat_S);
				memcpy(ptr_prtcls,ptr_prtclsOut,size_Mat);
				memcpy(ptr_wtIn,ptr_wtOut,size_wt);
			}
	//		cout << "before ESP_PF\n";
	//		memcpy(&stateOut,ptr_stateOut,size_Mat_S);
	//		showmat_S(&stateOut);

			// Data will be migrated to kernel space
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_obsFP,buffer_pxxFP,buffer_stateFP,buffer_wt,buffer_prtcls},0/* 0 means from host*/));
			//Launch the Kernel
			OCL_CHECK(err, err = q.enqueueTask(krnl_ESP_PF));
			// The result of the previous kernel execution will need to be retrieved in
			// order to view the results. This call will transfer the data from FPGA to
			// source_results vector
			OCL_CHECK(err, q.enqueueMigrateMemObjects({buffer_stateOut,buffer_pxxOut,buffer_prtclsOut,buffer_wtOut},CL_MIGRATE_MEM_OBJECT_HOST));

			OCL_CHECK(err, q.finish());
			fixed_type wtOut_data[NUM_PARTICLES];
			memcpy(wtOut_data,ptr_wtOut,size_wt);
			memcpy(&stateOut,ptr_stateOut,size_Mat_S);
			memcpy(&pxxOut,ptr_pxxOut,size_Mat_S);
			step = step +1;
			std::string fol_dir = "";
			write_csv(Save_Path(fol_dir,"PLv1_pxx",NUM_PARTICLES),convert_double(pxxOut.entries,NUM_VAR,NUM_VAR),NUM_VAR,NUM_VAR);
			write_csv(Save_Path(fol_dir,"PLv1_state",NUM_PARTICLES),convert_double(stateOut.entries,NUM_VAR,1),NUM_VAR,1);

			cout << stateOut.entries[0] <<"," << stateOut.entries[13] <<"\n";
			cout << "end of step "<<step<<" \n";
		}
    }
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_obsFP , ptr_obs));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_pxxFP , ptr_pxxIn));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_stateFP , prt_stateIn));
	OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_wt , ptr_wtIn));
	OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_prtcls , ptr_prtcls));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_stateOut , ptr_stateOut));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_pxxOut , ptr_pxxOut));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_prtclsOut , ptr_prtclsOut));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_wtOut , ptr_wtOut));

    OCL_CHECK(err, err = q.finish());
    int match =0;

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl; 
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);

}

