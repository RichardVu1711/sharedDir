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
#include "Cal/GISPzx.h"
#include "mvnpdf/mvnpdf_code.h"
#include "rk4/rk4.h"
#include "GISmsmt_prcs.h"

static const std::string error_message =
    "Error: Result mismatch:\n"
    "i = %d CPU result = %d Device result = %d\n";
void wait_for_enter(const std::string &msg) {
    std::cout << msg << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
int main(int argc, char* argv[]) {

    //TARGET_DEVICE macro needs to be passed from gcc command line
    if(argc != 2) {
		std::cout << "Usage: " << argv[0] <<" <xclbin>" << std::endl;
		return EXIT_FAILURE;
	}
//    xrt::profile::user_range range("Phase 0", "Intialization");

//	xrt::profile::user_event events;
//
//	std::string binaryFile = argv[1];
    std::string xclbinFilename = argv[1];
    
    // Compute the size of array in bytes
    size_t size_Mat_S = 1 * sizeof(Mat_S);
    size_t size_Mat = 1 * sizeof(Mat);
    size_t size_wt = NUM_PARTICLES*sizeof(fixed_type);
    size_t size_rndRk4 = 4*NUM_VAR*sizeof(fixed_type);
    size_t size_rndSigMa = NUM_VAR*NUM_PARTICLES*sizeof(fixed_type);
    size_t size_zDiff = NUM_PARTICLES*6*1*sizeof(fixed_type);
    size_t size_pzx = NUM_PARTICLES*6*6*sizeof(fixed_type);
    size_t size_msmt = 1*sizeof(msmt);
    size_t size_pxx = NUM_VAR*NUM_VAR*sizeof(fixed_type);
    // Creates a vector of DATA_SIZE elements with an initial value of 10 and 32
    // using customized allocator for getting buffer alignment to 4k boundary
    
    std::vector<cl::Device> devices;
    cl::Device device;
    cl_int err;
    cl::Context context;
    cl::CommandQueue q;

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
    
    cout << "Creating Program from Binary File\n";
    cl::Program::Binaries bins;
    bins.push_back({buf,nb});
    devices.resize(1);
    OCL_CHECK(err, program = cl::Program(context, devices, bins, NULL, &err));

//	cl::Kernel krnl_CalPzxZdiff;
//	cl::Kernel krnl_PFupdate;
    cout << "Create kernel: krnl_sigmaComp,krnl_ESPCrtParticles,krnl_mean_X_and_Pxx,krnl_CalPzxZdiff, and krnl_PFupdate\n";
    OCL_CHECK(err, cl::Kernel krnl_sigmaComp = cl::Kernel(program,"sigmaComp", &err));
    OCL_CHECK(err, cl::Kernel krnl_ESPCrtParticles = cl::Kernel(program,"ESPCrtParticles", &err));
    OCL_CHECK(err, cl::Kernel krnl_mean_X_and_Pxx = cl::Kernel(program,"mean_X_and_Pxx", &err));
    OCL_CHECK(err, cl::Kernel krnl_CalPzxZdiff = cl::Kernel(program,"CalPzxZdiff", &err));
    OCL_CHECK(err, cl::Kernel krnl_PFupdate = cl::Kernel(program,"PFupdate", &err));

    // This call will get the kernel object from program. A kernel is an 
    // OpenCL function that is executed on the FPGA. 

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

    fixed_type Neff_data[52];
    Mat_S stateOut,pxxOut;
    Mat prtclsOut;
    cout << "create buffer\n";
    // input buffers initialized
    OCL_CHECK(err, cl::Buffer buffer_obsIn(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_pxxIn(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_stateIn(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_wtIn(context, CL_MEM_READ_ONLY, size_wt, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_prtclsIn(context, CL_MEM_READ_ONLY, size_Mat, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_sigmaIn(context, CL_MEM_READ_ONLY, size_Mat, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_rndRk4(context, CL_MEM_READ_ONLY, size_rndRk4, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_rndSigMa(context, CL_MEM_READ_ONLY, size_rndSigMa, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_stateInPro(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_msmtInfo(context, CL_MEM_READ_ONLY, size_msmt, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_avgPxxIn(context, CL_MEM_READ_ONLY, size_pxx, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_RMat(context, CL_MEM_READ_ONLY, size_Mat_S, NULL, &err));


    // output buffers initialized
    OCL_CHECK(err, cl::Buffer buffer_stateOut(context, CL_MEM_WRITE_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_pxxOut(context, CL_MEM_WRITE_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_prtclsOut(context, CL_MEM_WRITE_ONLY, size_Mat, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_wtOut(context, CL_MEM_WRITE_ONLY, size_wt, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_sigmaOut(context, CL_MEM_WRITE_ONLY, size_Mat, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_stateOutPro(context, CL_MEM_WRITE_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_zDiffOut(context, CL_MEM_WRITE_ONLY, size_zDiff, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_pzxOut(context, CL_MEM_WRITE_ONLY, size_pzx, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_avgStateOut(context, CL_MEM_WRITE_ONLY, size_Mat_S, NULL, &err));
    OCL_CHECK(err, cl::Buffer buffer_avgPxxOut(context, CL_MEM_WRITE_ONLY, size_Mat_S, NULL, &err));
    int step =0;


    //set the kernel Arguments
    int narg=0;
    //sigmaComp function
    // sigma(pxx,sigma,rndSigMa)
    // pxx = 13x13, sigma = 13x1024, and rndSigMa  = 13x1024
    cout << "set kernel arguments for: krnl_sigmaComp,krnl_ESPCrtParticles\n";
    OCL_CHECK(err, err = krnl_sigmaComp.setArg(0,buffer_pxxIn));
    OCL_CHECK(err, err = krnl_sigmaComp.setArg(2,buffer_rndSigMa));
    OCL_CHECK(err, err = krnl_sigmaComp.setArg(1,buffer_sigmaOut));


    //ESPCrtParticles function - ESPCrtParticles(state_pro, sigma,prtclsOut)
    // state_pro = 13x1, sigma = 13x1024,prtclsOut
    OCL_CHECK(err, err = krnl_ESPCrtParticles.setArg(0,buffer_stateInPro));
    OCL_CHECK(err, err = krnl_ESPCrtParticles.setArg(1,buffer_sigmaIn));
    OCL_CHECK(err, err = krnl_ESPCrtParticles.setArg(2,buffer_prtclsOut));
//
    //mean_X_and_Pxx function - mean_X_and_Pxx(prtclsIn, avg_x,avg_pxx);
    OCL_CHECK(err, err = krnl_mean_X_and_Pxx.setArg(0,buffer_prtclsIn));
    OCL_CHECK(err, err = krnl_mean_X_and_Pxx.setArg(1,buffer_avgPxxOut));
//
    //CalPzxZdiff function - CalPzxZdiff(prtclsIn,msmtinfo,step,avg_pxxIn,zDiff,pzx)
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(0,buffer_prtclsIn));
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(1,buffer_msmtInfo));
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(2,buffer_msmtInfo));
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(3,buffer_msmtInfo));
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(4,buffer_RMat));
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(5,step));
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(6,buffer_avgPxxIn));
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(7,buffer_zDiffOut));
    OCL_CHECK(err, err = krnl_CalPzxZdiff.setArg(8,buffer_pzxOut));

    //PFUpdate function - PFupdate(particle, wt,  pxx, state,s tateOut, pxxOut)
    OCL_CHECK(err, err = krnl_PFupdate.setArg(0,buffer_prtclsIn));
    OCL_CHECK(err, err = krnl_PFupdate.setArg(1,buffer_wtIn));
    OCL_CHECK(err, err = krnl_PFupdate.setArg(2,buffer_stateOut));
    OCL_CHECK(err, err = krnl_PFupdate.setArg(3,buffer_pxxOut));
    // PFUpdate will be worried later on.
    //We then need to map our OpenCL buffers to get the pointers
    int *ptr_obs,*ptr_pxxIn,*prt_stateIn,*ptr_wtIn,*ptr_prtclsIn, *ptr_RMat,
		*ptr_sigmaIn,*ptr_rndRk4,*ptr_rndSigma,*ptr_stateInPro,*ptr_msmtInfo,*ptr_avgPxxIn,
		*ptr_stateOut, *ptr_pxxOut,*ptr_prtclsOut,*ptr_wtOut,
		*ptr_sigmaOut,*ptr_stateOutPro,*ptr_zDiffOut,*ptr_pzxOut,*ptr_avgStateOut,*ptr_avgPxxOut;

    // ptr Inputs
    OCL_CHECK(err, ptr_obs = (int*)q.enqueueMapBuffer (buffer_obsIn, CL_TRUE , CL_MAP_WRITE , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_pxxIn = (int*)q.enqueueMapBuffer (buffer_pxxIn , CL_TRUE , CL_MAP_WRITE , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, prt_stateIn = (int*)q.enqueueMapBuffer (buffer_stateIn , CL_TRUE , CL_MAP_WRITE , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_wtIn = (int*)q.enqueueMapBuffer (buffer_wtIn , CL_TRUE , CL_MAP_WRITE , 0, size_wt, NULL, NULL, &err));
    OCL_CHECK(err, ptr_prtclsIn = (int*)q.enqueueMapBuffer (buffer_prtclsIn , CL_TRUE , CL_MAP_WRITE , 0, size_Mat, NULL, NULL, &err));

    OCL_CHECK(err, ptr_sigmaIn = (int*)q.enqueueMapBuffer (buffer_sigmaIn , CL_TRUE , CL_MAP_WRITE , 0, size_Mat, NULL, NULL, &err));
    OCL_CHECK(err, ptr_rndSigma = (int*)q.enqueueMapBuffer (buffer_rndSigMa , CL_TRUE , CL_MAP_WRITE , 0, size_rndSigMa, NULL, NULL, &err));
    OCL_CHECK(err, ptr_stateInPro = (int*)q.enqueueMapBuffer (buffer_stateInPro , CL_TRUE , CL_MAP_WRITE , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_msmtInfo = (int*)q.enqueueMapBuffer (buffer_msmtInfo , CL_TRUE , CL_MAP_WRITE , 0, size_msmt, NULL, NULL, &err));
    OCL_CHECK(err, ptr_avgPxxIn = (int*)q.enqueueMapBuffer (buffer_avgPxxIn , CL_TRUE , CL_MAP_WRITE , 0, size_pxx, NULL, NULL, &err));
    OCL_CHECK(err, ptr_RMat = (int*)q.enqueueMapBuffer (buffer_RMat , CL_TRUE , CL_MAP_WRITE , 0, size_Mat_S, NULL, NULL, &err));

    // ptr Output
    OCL_CHECK(err, ptr_stateOut = (int*)q.enqueueMapBuffer (buffer_stateOut , CL_TRUE , CL_MAP_READ , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_pxxOut = (int*)q.enqueueMapBuffer (buffer_pxxOut , CL_TRUE , CL_MAP_READ , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_prtclsOut = (int*)q.enqueueMapBuffer (buffer_prtclsOut , CL_TRUE , CL_MAP_READ , 0, size_Mat, NULL, NULL, &err));
    OCL_CHECK(err, ptr_wtOut = (int*)q.enqueueMapBuffer (buffer_wtOut , CL_TRUE , CL_MAP_READ , 0, size_wt, NULL, NULL, &err));

    OCL_CHECK(err, ptr_sigmaOut = (int*)q.enqueueMapBuffer (buffer_sigmaOut , CL_TRUE , CL_MAP_READ , 0, size_Mat, NULL, NULL, &err));
//    OCL_CHECK(err, ptr_stateOutPro = (int*)q.enqueueMapBuffer (buffer_stateOutPro , CL_TRUE , CL_MAP_READ , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_zDiffOut = (int*)q.enqueueMapBuffer (buffer_zDiffOut , CL_TRUE , CL_MAP_READ , 0, size_zDiff, NULL, NULL, &err));
    OCL_CHECK(err, ptr_pzxOut = (int*)q.enqueueMapBuffer (buffer_pzxOut , CL_TRUE , CL_MAP_READ , 0, size_pzx, NULL, NULL, &err));
    OCL_CHECK(err, ptr_avgStateOut = (int*)q.enqueueMapBuffer (buffer_avgStateOut , CL_TRUE , CL_MAP_READ , 0, size_Mat_S, NULL, NULL, &err));
    OCL_CHECK(err, ptr_avgPxxOut = (int*)q.enqueueMapBuffer (buffer_avgPxxOut , CL_TRUE , CL_MAP_READ , 0, size_Mat_S, NULL, NULL, &err));
    rand_init(0);


    for(int i_run = 0; i_run < 1;i_run++)
    {
		int num_loop = 1;
		step =0;
	//    range.end();
	//
	//	events.mark("Initializes Variables and Data Test");


		for(int k=0; k < num_loop;k++)
		{
	//    	range.start("Phase 1", "Set Up Pxx, Particles, state and Obs datas");
			for(int i =0; i < 10;i ++)
			{
				obs.entries[i] = obs_data[step*10 + i];
			}
			std::string fol_dir = "";
			memcpy(ptr_obs,&obs,size_Mat_S);
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
				memcpy(ptr_prtclsIn,&prtclsIn,size_Mat);
			}
			else
			{
				memcpy(ptr_pxxIn,ptr_pxxOut,size_Mat_S);
				memcpy(&pxxIn,ptr_stateOut,size_Mat_S);
				memcpy(prt_stateIn,ptr_stateOut,size_Mat_S);
				memcpy(&stateIn,ptr_stateOut,size_Mat_S);
				memcpy(ptr_prtclsIn,ptr_prtclsOut,size_Mat);
				memcpy(ptr_wtIn,ptr_wtOut,size_wt);
			}

	//		range.end();
	//		range.start("Phase 2", "Create random numbers");

			cout << "PS: Create random number for Sigma\n";
			fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES];
			fixed_type rnd_rk4[NUM_VAR*4];
			fixed_type rnd_rsmp;
			// generate random numbers for sigma computation
			for(int i=0; i< NUM_VAR*NUM_PARTICLES;i++)
			{
				double rnd_temp;
				randn(&rnd_temp,0,0);
				rnd_sigma[i] = rnd_temp;
			}
			// generate random numbers for rk4
			for(int i=0; i< NUM_VAR*4;i++)
			{
				double rnd_temp;
				randn(&rnd_temp,0,0);
				rnd_rk4[i] = rnd_temp;
			}

			// generate random numbers for resample
	//		double rnd_temp;
	//		randn(&rnd_temp,0,0);
	//		rnd_rsmp = rnd_temp;
	//		range.end();
			cout << "PL: Sigma\n";
			memcpy(ptr_rndSigma,rnd_sigma,size_rndSigMa);
			//sigmaComp function
			// Data will be migrated to kernel space
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_pxxIn,buffer_rndSigMa},0/* 0 means from host*/));
			//Launch the Kernel
			OCL_CHECK(err, err = q.enqueueTask(krnl_sigmaComp));
			// The result of the previous kernel execution will need to be retrieved in
			// order to view the results. This call will transfer the data from FPGA to
			// source_results vector
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_sigmaOut},CL_MIGRATE_MEM_OBJECT_HOST));
			OCL_CHECK(err, q.finish());

			Mat sigma;
			newmat(&sigma,NUM_VAR,NUM_PARTICLES);
			memcpy(&sigma,ptr_sigmaOut,size_rndSigMa);
//			write_csv(Save_Path(fol_dir,"sigmaOut_host",NUM_PARTICLES),convert_double(sigma.entries,13,NUM_PARTICLES,1),13,NUM_PARTICLES);

			cout << "PS: rk4 -> Propagate Mean State\n";
			Mat_S stateIn_pro;
			init_mat(&stateIn_pro,NUM_VAR,1);
			rk4(&stateIn,&stateIn_pro,rnd_rk4);
	//		showmat_S(&stateIn_pro);

			// Extrapolate the particles based on the mean propagation
			//PL********************************************//
			// ESPCrtParticles(&stateIn_pro,&sigMa,&prtclsOut);
			cout << "PL: ESPCrtParticles -> Extrapolate Particles\n";
			memcpy(ptr_stateInPro,&stateIn_pro,size_Mat_S);
			memcpy(ptr_sigmaIn,ptr_sigmaOut,size_Mat);
			//ESPCrtParticles function
			// Data from PS -> PL
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_stateInPro,buffer_sigmaIn},0/* 0 means from host*/));
			//Launch the Kernel
			OCL_CHECK(err, err = q.enqueueTask(krnl_ESPCrtParticles));
			// data from PL -> PS
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_prtclsOut},CL_MIGRATE_MEM_OBJECT_HOST));
			OCL_CHECK(err, q.finish());

			//**********************************************//
			memcpy(&prtclsIn,ptr_prtclsOut,size_Mat);
//			write_csv(Save_Path(fol_dir,"prtclsOut_host",NUM_PARTICLES),convert_double(prtclsIn.entries,13,NUM_PARTICLES,1),13,NUM_PARTICLES);



			//PL********************************************//
			// Calculate the averaged state and covariance
			// mean_X_and_Pxx(&prtclsOut,&X_avg,&Pxx_);
			cout << "PL: mean_X_and_Pxx -> calculate averaged state and covariance\n";
			memcpy(ptr_prtclsIn,ptr_prtclsOut,size_Mat);
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_prtclsIn},0/* 0 means from host*/));
			//Launch the Kernel
			OCL_CHECK(err, err = q.enqueueTask(krnl_mean_X_and_Pxx));
			// data from PL -> PS
			OCL_CHECK(err, q.enqueueMigrateMemObjects({buffer_avgPxxOut},CL_MIGRATE_MEM_OBJECT_HOST));
			OCL_CHECK(err, q.finish());
			//**********************************************//
			Mat_S Pxx_;
			init_mat(&Pxx_,NUM_VAR,NUM_VAR);
			memcpy(&Pxx_,ptr_avgPxxOut,size_Mat_S);
//			write_csv(Save_Path(fol_dir,"Pxx__host",NUM_PARTICLES),convert_double(Pxx_.entries,13,13,0),13,13);
//			for(int i=0; i < 169;i++)
//			{
//				Pxx_.entries[i] = i+1;
//			}
//			Pxx_.entries[7] = 17;
			cout << "PS: msmt_prces -> Extract data \n";
			msmt msmtinfo = msmt_prcs(&obs);
			Mat_S R = R_cal(msmtinfo.n_aoa,msmtinfo.n_tdoa);
			//PL********************************************//
			// Calculate pzx and zdiff to prepare for the mvnpdf function
			// CalPzxZdiff(&prtclsOut,&msmtinfo,step,&Pxx_,zDiff,pzx);
			cout << "PL: CalPzxZdiff -> Calculate pzx and zDiff \n";
			// ptrclsIn is from the previous cal
			// msmtInfo is from msmt_prcs Output
			// avgPxxIn is from the PL:krnl_mean_X_and_Pxx, and copied from ptr_pxxOut
			// data from PS -> PL
			memcpy(ptr_msmtInfo,&msmtinfo,size_msmt);
			memcpy(ptr_avgPxxIn,Pxx_.entries,size_pxx);
			memcpy(ptr_RMat, R.entries,size_Mat_S);
//			memcpy(ptr_avgPxxIn,Pxx_.entries,size_Mat_S);
//			ptr_avgPxxIn[0] = 17;
			wait_for_enter("Wait for Triggering");
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_prtclsIn,buffer_msmtInfo,buffer_avgPxxIn},0/* 0 means from host*/));
			// Launch the Kernel
			OCL_CHECK(err, err = q.enqueueTask(krnl_CalPzxZdiff));
			// data from PL -> PS
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_zDiffOut,buffer_pzxOut},0/* 0 means from host*/));
			OCL_CHECK(err, q.finish());
			fixed_type zDiff[NUM_PARTICLES*SN_NUM*2];
			fixed_type pzx[NUM_PARTICLES*SN_NUM*2*SN_NUM*2];
			memcpy(zDiff,ptr_zDiffOut,size_zDiff);
			memcpy(pzx,ptr_pzxOut,size_pzx);
			write_csv("pzx.csv",convert_double(pzx,1,NUM_PARTICLES*36,-1),6*1024,6);
			write_csv("zDiff.csv",convert_double(zDiff,1,NUM_PARTICLES*6,-1),1024,6);

			// PS: mvnpdf
	//		range.start("mvnpdf", "calculate mvnpdf");
			cout << "PS: mvnpdf -> calculate the likelihood values\n";
			fixed_type sum_fp =0;
			int n_obs = msmtinfo.n_aoa + msmtinfo.n_tdoa;
			for(int i=0; i <NUM_PARTICLES;i++)
			{
				double zDiff_du[SN_NUM*2];
				double pzx_du[SN_NUM*2][SN_NUM*2];
				double Mu[SN_NUM*2] = {0,0,0,0,0,0};
				for(int i1=0; i1< SN_NUM*2;i1++)
				{
					zDiff_du[i1] = zDiff[i*SN_NUM*2+i1];
					for(int i2=0; i2< SN_NUM*2;i2++)
					{
						pzx_du[i1][i2] = pzx[i*SN_NUM*2*SN_NUM*2 + i1*SN_NUM*2+i2];
					}
				}
				double p_du = mvnpdf_code(zDiff_du, Mu,pzx_du,n_obs);
	//			cout << p_du << "\t";
				fixed_type p_fp = p_du;
				wtIn[i] = wtIn[i]*p_fp;
				sum_fp = sum_fp + wtIn[i];
			}
			for(int i =0; i < NUM_PARTICLES;i++)
			{
				wtIn[i] = wtIn[i]/sum_fp;
	//	    	cout << wtOut[i] << "\t";
			}
	//		range.end();

	//		write_csv(Save_Path(fol_dir,"wtIn_host",NUM_PARTICLES),convert_double(wtIn,1,NUM_PARTICLES,1),1,NUM_PARTICLES);
	//		range.start("Resample", "Evaluate the estimation quality");
			double N_eff=0;
			double re_sum =0;
			for(int j =0; j < NUM_PARTICLES;j++)
			{
				double temp2 = wtIn[j];
				re_sum += (temp2*temp2);
			}
			N_eff = 1/re_sum;
			Mat prtclsOut;
			newmat(&prtclsOut,NUM_VAR,NUM_PARTICLES);
			memcpy(&prtclsOut,ptr_prtclsOut,size_Mat);


			if(N_eff < NUM_PARTICLES*0.5 || N_eff > 10000000000)
			{
				fixed_type rnd_rsmp;
				double rnd_temp;
				randn(&rnd_temp,0,0);
				rnd_rsmp = rnd_temp;
				resamplePF_wrap(&prtclsOut,wtIn,rnd_rsmp);
				memcpy(ptr_prtclsOut,&prtclsOut,size_Mat);
				cout << "resample required\n";
			}
	//		range.end();

			// !!! Require to change -> does not need state and covariance inputs !!!
			//PL********************************************//
			// Update the new state
			// void PFupdate(Mat* particle, fixed_type wt[NUM_PARTICLES], Mat_S* pxx,Mat_S* state,
			//	 		 	Mat_S* stateOut,Mat_S* pxxOut)
			cout << "PL: PFupdate -> Update the mean state and covariance\n";

			memcpy(ptr_wtIn,wtIn,size_wt);
			OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_prtclsIn,buffer_wtIn,buffer_pxxIn,buffer_stateIn},0/* 0 means from host*/));
			//Launch the Kernel
			OCL_CHECK(err, err = q.enqueueTask(krnl_PFupdate));
			// data from PL -> PS
			OCL_CHECK(err, q.enqueueMigrateMemObjects({buffer_stateOut,buffer_pxxOut},CL_MIGRATE_MEM_OBJECT_HOST));
			OCL_CHECK(err, q.finish());
			//**********************************************//

			Mat_S stateOut,pxxOut;
			memcpy(&stateOut,ptr_stateOut,size_Mat_S);
			memcpy(&pxxOut,ptr_pxxOut,size_Mat_S);
			Neff_data[step] = N_eff;
			write_csv(Save_Path(fol_dir,"PSPLv14_state",NUM_PARTICLES),convert_double(stateOut.entries,NUM_VAR,1,0),NUM_VAR,1);
			write_csv(Save_Path(fol_dir,"PSPLv14_Pxx",NUM_PARTICLES),convert_double(pxxOut.entries,NUM_VAR,NUM_VAR,0),NUM_VAR,NUM_VAR);
			cout << stateOut.entries[0] <<"," << stateOut.entries[13]<<","<<N_eff <<"\n";
			cout << "end of step "<<step<< " - run " << i_run<<" \n";
			step = step +1;

		}
		write_csv(Save_Path("","Neff_host100",NUM_PARTICLES),convert_double(Neff_data,1,52,1),1,52);
    }
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_obsIn , ptr_obs));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_pxxIn , ptr_pxxIn));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_stateIn , prt_stateIn));
	OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_wtIn , ptr_wtIn));
	OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_prtclsIn , ptr_prtclsIn));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_stateOut , ptr_stateOut));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_pxxOut , ptr_pxxOut));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_prtclsOut , ptr_prtclsOut));
    OCL_CHECK(err, err = q.enqueueUnmapMemObject(buffer_wtOut , ptr_wtOut));

    OCL_CHECK(err, err = q.finish());
    int match =0;

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl; 
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);

}

