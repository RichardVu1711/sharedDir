#include "host_setup.h"

size_t size_Mat = 1 * sizeof(Mat);
size_t size_Mat_S = 1 * sizeof(Mat_S);
size_t size_wt = NUM_PARTICLES*sizeof(fixed_type);

size_t size_pxx = NUM_VAR*NUM_VAR*sizeof(fixed_type);
size_t size_large = NUM_PARTICLES*NUM_VAR*sizeof(fixed_type);
size_t size_state = NUM_VAR*sizeof(fixed_type);

size_t size_msmt = 1*sizeof(msmt);
size_t size_Rmat = N_MEAS*sizeof(fixed_type);

size_t size_zDiff = NUM_PARTICLES*N_MEAS*sizeof(fixed_type);
size_t size_pzx = NUM_PARTICLES*N_MEAS*N_MEAS*sizeof(fixed_type);

int device_setup(int argc, char* argv[],
				std::vector<cl::Device>& devices_lc,
			    cl::Device& device_lc,
			    cl::Context& context_lc,
			    cl::CommandQueue q_lc[Q_LEN],
			    cl::Program& program_lc)
{
	 //TARGET_DEVICE macro needs to be passed from gcc command line
	    if(argc != 2) {
			std::cout << "Usage: " << argv[0] <<" <xclbin>" << std::endl;
			return EXIT_FAILURE;
		}
	    cl_int err;
	    std::string xclbinFilename = argv[1];
		std::vector<cl::Platform> platforms_lc;
	    bool found_device = false;

	    //traversing all Platforms To find Xilinx Platform and targeted
	    //Device in Xilinx Platform
	    cl::Platform::get(&platforms_lc);
	    for(size_t i = 0; (i < platforms_lc.size() ) & (found_device == false) ;i++){
	        cl::Platform platform = platforms_lc[i];
	        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
	        if ( platformName == "Xilinx"){
	            devices_lc.clear();
	            platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices_lc);
		    if (devices_lc.size()){
			    device_lc = devices_lc[0];
			    found_device = true;
			    break;
		    }
	        }
	    }
	    if (found_device == false){
	       std::cout << "Error: Unable to find Target Device "
	           << device_lc.getInfo<CL_DEVICE_NAME>() << std::endl;
	       return EXIT_FAILURE;
	    }

	    // Creating Context and Command Queue for selected device
	    OCL_CHECK(err, context_lc = cl::Context(device_lc, NULL, NULL, NULL, &err));
	    for(int i=0; i < Q_LEN;i++)
	    {
	    	OCL_CHECK(err, q_lc[i] = cl::CommandQueue(context_lc, device_lc, CL_QUEUE_PROFILING_ENABLE, &err));
	    }

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
	    devices_lc.resize(1);
	    OCL_CHECK(err, program_lc = cl::Program(context_lc, devices_lc, bins, NULL, &err));
	    return 0;
}

int buf_link(int** ptr, cl::Buffer& buf, size_t in_size, int isWrite, int qIdx)
{
	//Create a sync link between point and buf with the given size
	//isWrite = 1 -> write, else read
	//This size is the configuration of the host, so from the host perspective
	//We "write" into the "read" buffer and "read" from the "write" buffer.
	cl_int err;
	if(isWrite)
	{
	    OCL_CHECK(err, ptr[0] = (int*)q[qIdx].enqueueMapBuffer (buf , CL_TRUE , CL_MAP_WRITE , 0, in_size, NULL, NULL, &err));
	}
	else
	{
	    OCL_CHECK(err, ptr[0] = (int*)q[qIdx].enqueueMapBuffer (buf , CL_TRUE , CL_MAP_READ , 0, in_size, NULL, NULL, &err));
	}
	return 0;
}


// events variable is required !!
int block_S(int** pM_pxxIn, fixed_type pxx[NUM_VAR*NUM_VAR],cl::Buffer &bM_pxxIn,
			int** p_stateIn, fixed_type state[NUM_VAR], cl::Buffer &b_stateIn, fixed_type rnd_rk4[4*NUM_VAR],
			int** p_sigMatIn, fixed_type sigMat[NUM_VAR*NUM_PARTICLES], cl::Buffer &b_sigMatIn,
			int** p_sigMatOut, cl::Buffer &b_sigMatOut,
			int** p_rndIn, fixed_type rnd[NUM_VAR*NUM_PARTICLES], cl::Buffer &b_rndIn,
			int** p_prtclsOut, fixed_type prtcls[NUM_VAR*NUM_PARTICLES], cl::Buffer &b_prtclsOut,
			cl::Kernel& kSigma,cl::Kernel& kCreate, int qIdx)
{
	// copy input for sigmaComp block

	memcpy(pM_pxxIn[0],pxx,size_pxx);
	memcpy(p_rndIn[0],rnd,size_large);
	cl_int err;

	// execute sigmaComp Block
	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({bM_pxxIn,b_rndIn},0/* 0 means from host*/));
	//Launch the Kernel
	OCL_CHECK(err, err = q[qIdx].enqueueTask(kSigma));
	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_sigMatOut},CL_MIGRATE_MEM_OBJECT_HOST));

	// starting triggering rk4
	fixed_type state_pro[NUM_VAR];	//essential variable for rk4
	Mat_S stateOut_pro;
	Mat_S M_state;
	M_state.col = 1;
	M_state.row = NUM_VAR;
	for(int i=0; i < 13;i++)
	{
		M_state.entries[i*NUM_VAR] = state[i];
	}
	rk4(&M_state,&stateOut_pro,rnd_rk4);

	// copy output of rk4
	for(int i=0; i < NUM_VAR;i++)
	{
		state_pro[i] = stateOut_pro.entries[i*NUM_VAR];
	}
	//"wait for the sigmaComp Block finalised.\n";
	OCL_CHECK(err, q[qIdx].finish());

	// NOTE: recording data is for debugging purpose, need to turn it to improve the overall performance !!
	memcpy(sigMat,p_sigMatOut[0],size_large);
	write_csv("/mnt/result/sigMat.csv",convert_double(sigMat,1,13*1024,0),13,1024);

	// Copy input for ESPCrtParticles
	memcpy(p_stateIn[0],state_pro,size_state);
	memcpy(p_sigMatIn[0],p_sigMatOut[0],size_large);

	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_stateIn,b_sigMatIn},0/* 0 means from host*/));
	OCL_CHECK(err, err = q[qIdx].enqueueTask(kCreate));
	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_prtclsOut},CL_MIGRATE_MEM_OBJECT_HOST));
	// NOTE  need to check if a queue is done or not, if not ignore whatever coming.
	OCL_CHECK(err, q[qIdx].finish());

	// NOTE: recording data is for debugging purpose, need to turn it to improve the overall performance !!
	memcpy(prtcls,p_prtclsOut[0],size_large);
	write_csv("/mnt/result/prtcls.csv",convert_double(prtcls,1,13*1024,0),13,1024);
	cout << "\nFinished Sampling\n";

	// indicating the block is done.
	return 1;

}

int block_C(int** p_prtclsOut,int** p_prtclsIn, cl::Buffer &b_prtclsIn,
			int** p_pxxOut, cl::Buffer &b_pxxOut,
			Mat_S* obs_data,
			int** p_msmtIn, cl::Buffer &b_msmtIn,
			int** p_RmatIn, cl::Buffer &b_RmatIn,
			int** p_pxxIn, cl::Buffer &b_pxxIn,
			int** p_zDiffOut, cl::Buffer &b_zDiffOut,
			int** p_pzxOut, cl::Buffer &b_pzxOut,
			int step, double* N_eff,
			fixed_type wt[NUM_PARTICLES],
			cl::Kernel k_mPxx, cl::Kernel kCal,
			int qIdx)
{
	cl_int err;

	memcpy(p_prtclsIn[0],p_prtclsOut[0],size_large);

	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_prtclsIn},0/* 0 means from host*/));
	OCL_CHECK(err, err = q[qIdx].enqueueTask(k_mPxx));
	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_pxxOut},CL_MIGRATE_MEM_OBJECT_HOST));

	OCL_CHECK(err, q[qIdx].finish());

	memcpy(p_pxxIn[0],p_pxxOut[0],size_pxx);

	msmt msmtinfo = msmt_prcs(obs_data);
	Mat_S Rmat = R_cal(&msmtinfo);
	fixed_type R [N_MEAS];
	for(int i=0; i < N_MEAS;i++)
	{
		R[i] = Rmat.entries[i*NUM_VAR+i];
	}

//	memcpy(p_RmatIn[0],R,size_Rmat);
	memcpy(p_msmtIn[0],&msmtinfo,size_msmt);
	memcpy(p_RmatIn[0],R,size_Rmat);

	memcpy(p_pxxIn[0],p_pxxOut[0],size_pxx);
	fixed_type pxx[NUM_VAR*NUM_VAR];
	memcpy(pxx,p_pxxOut[0],size_pxx);
	cout << "\npzx: " << pxx[0] <<", "<< pxx[14] << "\n";

    OCL_CHECK(err, err = kCal.setArg(3,step));
	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_prtclsIn,b_msmtIn,b_RmatIn,b_pxxIn},0/* 0 means from host*/));
	OCL_CHECK(err, err = q[qIdx].enqueueTask(kCal));
	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_zDiffOut,b_pzxOut},CL_MIGRATE_MEM_OBJECT_HOST));
	OCL_CHECK(err, q[qIdx].finish());

	fixed_type zDiff[NUM_PARTICLES*N_MEAS];
	fixed_type pzx[NUM_PARTICLES*N_MEAS*N_MEAS];

	memcpy(zDiff,p_zDiffOut[0],size_zDiff);
	memcpy(pzx,p_pzxOut[0],size_pzx);

// mvnpdf
	fixed_type sum_fp =0;
	int n_obs = msmtinfo.n_aoa + msmtinfo.n_tdoa;
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
				pzx_du[i1][i2] = pzx[i*N_MEAS*N_MEAS + i1*N_MEAS+i2];
			}
		}
		double p_du = mvnpdf_code(zDiff_du, Mu,pzx_du,n_obs);

		fixed_type p_fp = p_du;
		wt[i] = wt[i]*p_fp;
		sum_fp = sum_fp + wt[i];
	}

	for(int i =0; i < NUM_PARTICLES;i++)
	{
		wt[i] = wt[i]/sum_fp;
	}

	double re_sum =0;
	for(int j =0; j < NUM_PARTICLES;j++)
	{
		double temp2 = wt[j];
		re_sum += (temp2*temp2);
	}
	N_eff[0] = 1/re_sum;

	//Note: This is for testing purpose only, please remove it once the functionality is confirmed.
	write_csv("/mnt/result/zDiff.csv",convert_double(zDiff,1,6*1024,-1),1024,6);
	write_csv("/mnt/result/pzx.csv",convert_double(pzx,1,36*1024,-1),6*1024,6);
	write_csv("/mnt/result/wt.csv",convert_double(wt,1,1*1024,-1),1,1024);

	cout << "\nFinished Calculate Pzx Zdiff\n";
//
//	cout << "\nPzx: " << pzx[0] << ", " << pzx[28] << ", " << pzx[35] << "\n";
//	cout << "\nzDiff: " << zDiff[0] << ", " << zDiff[1] << ", " << zDiff[4] << "\n";
//	cout << "\nWt: " << wt[0] << ", " << wt[1] << ", " << wt[2] << "\n";

	return 0;

}

int block_R(fixed_type prtcls[NUM_VAR*NUM_PARTICLES], fixed_type wt[NUM_PARTICLES], double N_eff,
			int** p_prtclsIn, cl::Buffer& b_prtclsIn,
			int** p_wtIn, cl::Buffer& b_wtIn,
			int** p_stateOut, cl::Buffer& b_stateOut,
			int** p_pxxOut, cl::Buffer& b_pxxOut,
			int** p_stateIn, int** p_pxxIn,
			int step,
			cl::Kernel& kPFU, int qIdx)
{
	cl_int err;
	if(N_eff < NUM_PARTICLES*0.5 || N_eff > DBL_MAX*0.5)
	{
		double rnd_temp;
		randn(&rnd_temp,0,0);
		fixed_type rnd_rsmp = rnd_temp;
		resamplePF_wrap(prtcls,wt,rnd_rsmp);
		memcpy(p_prtclsIn[0],&prtcls,size_large);
		cout << "resample required\n";
	}
	memcpy(p_wtIn[0], wt,size_wt);

	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_prtclsIn,b_wtIn},0/* 0 means from host*/));
	OCL_CHECK(err, err = q[qIdx].enqueueTask(kPFU));
	OCL_CHECK(err, err = q[qIdx].enqueueMigrateMemObjects({b_stateOut,b_pxxOut},CL_MIGRATE_MEM_OBJECT_HOST));
	OCL_CHECK(err,err = q[qIdx].finish());

	memcpy(p_stateIn[0],p_stateOut[0],size_state);
	memcpy(p_pxxIn[0],p_pxxOut[0],size_pxx);


	fixed_type state[NUM_VAR];
	fixed_type pxx[NUM_VAR*NUM_VAR];

	// This code is requiremment as we need to store the solution!
	memcpy(state,p_stateOut[0],size_state);
	memcpy(pxx,p_pxxOut[0],size_pxx);

	write_csv("/mnt/result/state_sol.csv",convert_double(state,1,13,-1),1,13);
	write_csv("/mnt/result/pxx_sol.csv",convert_double(pxx,1,13*13,-1),13,13);

	cout << "\nstate= " << state[0] << ", " << state[1] << "\t pxx =" << pxx[0] << ", " << pxx[14] << "\n";
	cout << "End of : " << step << " with Neff =  "<< N_eff <<".\n";
	return 0;
}
