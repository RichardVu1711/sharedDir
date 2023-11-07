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

int S_status =0; // 0 means available
int C_status =0;
int R_status =0;

cl::Event status1;
cl::Event status2;
cl::Event status3;
cl::Event status;

int done =0;
pthread_t t;

std::vector<cl::Event> waitList;

xrt::profile::user_range range;
xrt::profile::user_event events;
// An event callback function that prints the operations performed by the OpenCL
// runtime.
void* wait_thread(void *thread_arg)
{
	cl_int err;
	cl::CommandQueue* q = (cl::CommandQueue*) thread_arg;
	cout << "\n Im inside a thread \n";
	OCL_CHECK(err, err = q[0].finish());
	done = 1;
	cout << "Thread is done \n";
	pthread_exit(NULL);

}
void event_cb(cl_event event1, cl_int cmd_status, void* data) {
    cl_int err;
    cl_command_type command;
    cl::Event event(event1, true);
    OCL_CHECK(err, err = event.getInfo(CL_EVENT_COMMAND_TYPE, &command));
    cl_int status;
    OCL_CHECK(err, err = event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &status));
    const char* command_str;
    const char* status_str;
    switch (command) {
        case CL_COMMAND_READ_BUFFER:
            command_str = "buffer read";
            break;
        case CL_COMMAND_WRITE_BUFFER:
            command_str = "buffer write";
            break;
        case CL_COMMAND_NDRANGE_KERNEL:
            command_str = "kernel";
            break;
        case CL_COMMAND_MAP_BUFFER:
            command_str = "kernel";
            break;
        case CL_COMMAND_COPY_BUFFER:
            command_str = "kernel";
            break;
        case CL_COMMAND_MIGRATE_MEM_OBJECTS:
            command_str = "buffer migrate";
            break;
        default:
            command_str = "unknown";
    }
    switch (status) {
        case CL_QUEUED:
            status_str = "Queued";
            break;
        case CL_SUBMITTED:
            status_str = "Submitted";
            break;
        case CL_RUNNING:
            status_str = "Executing";
            break;
        case CL_COMPLETE:
            status_str = "Completed";
            break;
    }
    printf("[%s]: %s %s\n", reinterpret_cast<char*>(data), status_str, command_str);
    fflush(stdout);
}

// Sets the callback for a particular event
void set_callback(cl::Event event, const char* queue_name) {
    cl_int err;
    OCL_CHECK(err, err = event.setCallback(CL_COMPLETE, event_cb, (void*)queue_name));
}

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
//	    	OCL_CHECK(err, q_lc[i] = cl::CommandQueue(context_lc, device_lc, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
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
			int** p_stateIn, fixed_type state[NUM_VAR], cl::Buffer &b_stateIn,
			int** p_sigMatIn, cl::Buffer &b_sigMatIn,
			int** p_sigMatOut, cl::Buffer &b_sigMatOut,
			int** p_rndIn, cl::Buffer &b_rndIn,
			int** p_prtclsOut, cl::Buffer &b_prtclsOut,
			cl::Kernel& kSigma,cl::Kernel& kCreate,
			state_t* s_state, int C_stt,
			int idx_s, cl::Event* status_S, cl::Event* status_S1)
{

	// copy input for sigmaComp block
	// BUG status, stt_1, or stt_2 won't be saved
	cl_int err;
	cl_int stt_1;
	fixed_type state_pro[NUM_VAR];	//essential variable for rk4

	if(S_status==0){
		if(s_state[0] == SAMP){
			S_status = 1;
			fixed_type rnd_rk4[4*NUM_VAR];
			fixed_type rnd[NUM_VAR*NUM_PARTICLES];
			rng(rnd_rk4,rnd);

			memcpy(&pM_pxxIn[0][0],pxx,size_pxx);
			memcpy(&p_rndIn[0][0],rnd,size_large);
			OCL_CHECK(err, err = kCreate.setArg(0,b_stateIn));
			OCL_CHECK(err, err = kCreate.setArg(1,b_sigMatIn));
			OCL_CHECK(err, err = kCreate.setArg(2,b_prtclsOut));
			// execute sigmaComp Block
			OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({bM_pxxIn,b_rndIn},0/* 0 means from host*/));
			OCL_CHECK(err, err = q[idx_s].enqueueTask(kSigma));
			OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_sigMatOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,status_S));

//			OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_stateIn,b_sigMatIn},0/* 0 means from host*/));
//			OCL_CHECK(err, err = q[idx_s].enqueueTask(kCreate));
//			OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,status_S));


			// starting triggering rk4
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
		}
	}
	else if(S_status == 1)
	{
		cl_int p1Done;
		OCL_CHECK(err, err = status_S[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &p1Done));
		if((p1Done == CL_COMPLETE)){
			S_status =2;
//			clReleaseEvent(status_S[0]);

		}
//		S_status =2;
	}
	//"wait for the sigmaComp Block finalised.\n";
	else if(S_status ==2)
	{
		S_status =3;
		fixed_type sigMat[NUM_VAR*NUM_PARTICLES];
		// NOTE: recording data is for debugging purpose, need to turn it to improve the overall performance !!
		memcpy(sigMat,&p_sigMatOut[0][0],size_large);
		write_csv("/mnt/result/sigMat.csv",convert_double(sigMat,1,13*1024,-1),1,13*1024);
		write_csv("/mnt/result/state_pro.csv",convert_double(state_pro,1,13*1,-1),1,13);

		// Copy input for ESPCrtParticles
		memcpy(p_stateIn[0],state_pro,size_state);
		memcpy(p_sigMatIn[0],p_sigMatOut[0],size_large);

		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_stateIn,b_sigMatIn},0/* 0 means from host*/));
		OCL_CHECK(err, err = q[idx_s].enqueueTask(kCreate));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,&status_S1[0]));
//		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({bM_pxxIn,b_rndIn},0/* 0 means from host*/));
//		OCL_CHECK(err, err = q[idx_s].enqueueTask(kSigma));
//		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_sigMatOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,status_S1));

	}
	else if(S_status ==3){
	// NOTE  need to check if a queue is done or not, if not ignore whatever coming.
		cl_int p2Done;
		OCL_CHECK(err, err = status_S1[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &p2Done));
		if((p2Done == CL_COMPLETE)){
			S_status =4;
			fixed_type prtcls[13*1024];
			memcpy(prtcls,p_prtclsOut[0],size_large);
			write_csv("/mnt/result/prtcls.csv",convert_double(prtcls,1,13*1024,-1),13,1024);
		}
	}
	else if(S_status ==4){
		if(idx_s == 0){
			s_state[idx_s] = CAL;
			S_status = 0;
		}
		else{
			//check if the next block is available
			if(C_status ==0){
				s_state[idx_s] =CAL;
				S_status =0;
			}
		}
	}
	else{
		cout << "\nSomething goes wrong @ Block_S\n";
		return -1;
	}

	// indicating the block is done.
	return S_status;

}

int block_C(int** p_prtclsOut,int** p_prtclsIn, cl::Buffer &b_prtclsIn, fixed_type prtcls[NUM_VAR*NUM_PARTICLES],
			int** p_pxxOut, cl::Buffer &b_pxxOut,
			fixed_type obs_data[10],
			int** p_msmtIn, cl::Buffer &b_msmtIn,
			int** p_RmatIn, cl::Buffer &b_RmatIn,
			int** p_pxxIn, cl::Buffer &b_pxxIn,
			int** p_zDiffOut, cl::Buffer &b_zDiffOut,
			int** p_pzxOut, cl::Buffer &b_pzxOut,
			int step, double* N_eff,
			fixed_type wt[NUM_PARTICLES],
			cl::Kernel k_mPxx, cl::Kernel kCal,
			state_t* state,	int r_stt,
			int idx_s)
{
	cl_int err;
	cl_int stt_1;
	msmt msmtinfo;
	Mat_S Rmat;
	fixed_type R [N_MEAS];
	fixed_type zDiff[NUM_PARTICLES*N_MEAS];
	fixed_type pzx[NUM_PARTICLES*N_MEAS*N_MEAS];
	if(C_status==0){
		if(state[0] == CAL){
			C_status = 1;
			memcpy(p_prtclsIn[0],p_prtclsOut[0],size_large);

			OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsIn},0/* 0 means from host*/));
			OCL_CHECK(err, err = q[idx_s].enqueueNDRangeKernel(k_mPxx,0,1,1,NULL,NULL));
			OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_pxxOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,&status2));
			waitList.push_back(status);

		}
	}
	else if(C_status==1)
	{
//		memcpy(prtcls,p_prtclsOut[0],size_large);
//		OCL_CHECK(err, err = status2.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &stt_1));
		if(done == 1) {
			C_status =2;
			fixed_type pxx[169];
			memcpy(pxx,p_pxxOut[0],size_pxx);
			cout << "Done is finalised";
			cout << "pxx " << pxx[0] << ", " << pxx[168] << "\n";

		}
		else if(done ==0)
		{
			pthread_create(&t, NULL, &wait_thread, (void*) &q[idx_s]);
			done = -1;
		}
		else
		{
			//done nothing. Keep waiting
		}
//		if(stt_1==CL_COMPLETE)
//		{
//		    C_status =2;
////			memcpy(p_pxxIn[0],p_pxxOut[0],size_pxx);
//		}
	}
	else if(C_status ==2)
	{
		C_status = 3;

		msmtinfo = msmt_prcs(obs_data);
		Rmat = R_cal(&msmtinfo);

		for(int i=0; i < N_MEAS;i++)
		{
			R[i] = Rmat.entries[i*NUM_VAR+i];
		}
		memcpy(p_msmtIn[0],&msmtinfo,size_msmt);
		memcpy(p_RmatIn[0],R,size_Rmat);
		memcpy(p_pxxIn[0],p_pxxOut[0],size_pxx);
		fixed_type pxx[169];
		memcpy(pxx,p_pxxOut[0],size_pxx);

		cout << "pxx " << pxx[0] << ", " << pxx[168] << "\n";
		OCL_CHECK(err, err = kCal.setArg(3,step));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsIn,b_msmtIn,b_RmatIn,b_pxxIn},0/* 0 means from host*/));
		OCL_CHECK(err, err = q[idx_s].enqueueNDRangeKernel(kCal,0,1,1,NULL,NULL));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_zDiffOut,b_pzxOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,&status2));

	}
	else if(C_status == 3)
	{
	    OCL_CHECK(err, err = status2.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &stt_1))


		if(done == 1) {
//	    if(stt_1==CL_COMPLETE) {
//	        OCL_CHECK(err, err = q[idx_s].finish());
	    	C_status =4;
	    	memcpy(zDiff,p_zDiffOut[0],size_zDiff);
			memcpy(pzx,p_pzxOut[0],size_pzx);
			cout << "pzx= " << pzx[0] << ", " << pzx[35] << "\n";
	    }
		else if(done ==0)
		{
			pthread_create(&t, NULL, &wait_thread, (void*) &q[idx_s]);
			done = -1;
		}

	}
	// NOTE START FROM HERE PLEASE
	// get the status is here!!!

	else if(C_status ==4)
	{
		C_status =5;
		fixed_type sum_fp =0;
		int n_obs = msmtinfo.n_aoa + msmtinfo.n_tdoa;
		for(int i=0; i <NUM_PARTICLES;i++){
			double zDiff_du[N_MEAS];
			double pzx_du[N_MEAS][N_MEAS];
			double Mu[N_MEAS] = {0,0,0,0,0,0};

			for(int i1=0; i1< N_MEAS;i1++){
				zDiff_du[i1] = zDiff[i*N_MEAS+i1];
				for(int i2=0; i2< N_MEAS;i2++){
					pzx_du[i1][i2] = pzx[i*N_MEAS*N_MEAS + i1*N_MEAS+i2];
				}
			}
			double p_du = mvnpdf_code(zDiff_du, Mu,pzx_du,n_obs);
			fixed_type p_fp = p_du;
			wt[i] = wt[i]*p_fp;
			sum_fp = sum_fp + wt[i];
		}

		for(int i =0; i < NUM_PARTICLES;i++){
			wt[i] = wt[i]/sum_fp;
		}

		double re_sum =0;
		for(int j =0; j < NUM_PARTICLES;j++){
			double temp2 = wt[j];
			re_sum += (temp2*temp2);
		}
		N_eff[0] = 1/re_sum;
		memcpy(prtcls,p_prtclsOut[0],size_large);
	}
	// status 6 in here to determine whether should we move into the next state
	else if(C_status ==5){
		if(idx_s ==0){
			// first block always can go forward
			state[0] = UP;
			C_status =0;
		}
		else
		{
			if((r_stt == 0)||(r_stt == 5)){
				state[0] = UP;
				C_status =0;
			}
			else{
				// wait in here and until the next block is available
			}
		}
	}

	else{
		cout << "\n There's a bug at Block_C \n";
		return -1;
	}


	//Note: This is for testing purpose only, please remove it once the functionality is confirmed.
	//write_csv("/mnt/result/zDiff.csv",convert_double(zDiff,1,6*1024,-1),1024,6);
	//write_csv("/mnt/result/pzx.csv",convert_double(pzx,1,36*1024,-1),6*1024,6);
	//write_csv("/mnt/result/wt.csv",convert_double(wt,1,1*1024,-1),1,1024);

//	cout << "\nPzx: " << pzx[0] << ", " << pzx[28] << ", " << pzx[35] << "\n";
//	cout << "\nzDiff: " << zDiff[0] << ", " << zDiff[1] << ", " << zDiff[4] << "\n";
//	cout << "\nWt: " << wt[0] << ", " << wt[1] << ", " << wt[2] << "\n";

	return C_status;

}

int block_R( fixed_type prtcls[NUM_VAR*NUM_PARTICLES],fixed_type wt[NUM_PARTICLES], double N_eff,
			int** p_prtclsIn, cl::Buffer& b_prtclsIn,
			int** p_wtIn, cl::Buffer& b_wtIn,
			int** p_stateOut, cl::Buffer& b_stateOut,
			int** p_pxxOut, cl::Buffer& b_pxxOut,
			int** p_stateIn, int** p_pxxIn,
			fixed_type state[NUM_VAR],
			fixed_type pxx[NUM_VAR*NUM_VAR],
			int step,
			cl::Kernel& kPFU,
			state_t* state_s,
			int idx_s)
{
	cl_int err;
	cl_int stt_1;
	if(R_status ==0)
	{
		C_status =0;
	    R_status =1;

		if(N_eff < NUM_PARTICLES*0.5 || N_eff > DBL_MAX*0.5){
			double rnd_temp;
			randn(&rnd_temp,0,0);
			fixed_type rnd_rsmp = rnd_temp;
			//THIS COULD CREATE A BUG AS p_prtclsIn is a read buffer.
			resamplePF_wrap(prtcls,wt,rnd_rsmp);
			memcpy(p_prtclsIn[0],prtcls,size_large);
			cout << "resample required\n";
		}
		memcpy(p_wtIn[0], wt,size_wt);
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsIn,b_wtIn},0/* 0 means from host*/));
		OCL_CHECK(err, err = q[idx_s].enqueueNDRangeKernel(kPFU,0,1,1,NULL,NULL));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_stateOut,b_pxxOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,&status3));
	}
	else if(R_status == 1){
	    OCL_CHECK(err, err = status3.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &stt_1));
		if(stt_1==CL_COMPLETE){
			memcpy(p_stateIn[0],p_stateOut[0],size_state);
			memcpy(p_pxxIn[0],p_pxxOut[0],size_pxx);
			R_status =3;
		}

	}
	else if(R_status ==3)
	{
	    R_status =4;
		stt_1 = CL_SUBMITTED;



		// This code is requiremment as we need to store the solution!
		memcpy(state,p_stateOut[0],size_state);
		memcpy(pxx,p_pxxOut[0],size_pxx);

		write_csv("/mnt/result/state_sol.csv",convert_double(state,1,13,-1),1,13);
		write_csv("/mnt/result/pxx_sol.csv",convert_double(pxx,1,13*13,-1),13,13);

		cout << "\nstate= " << state[0] << ", " << state[1] << "\t pxx =" << pxx[0] << ", " << pxx[14] << "\n";
		cout << "End of : " << step << " with Neff =  "<< N_eff <<" at src = " << idx_s << " .\n";
	}
	else if(R_status ==4)
	{
		R_status =0;
		state_s[idx_s] = IDLE;
	}
	return R_status;
}

void rng(fixed_type rnd_rk4[NUM_VAR],
		 fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES])
{
	for(int i=0; i< NUM_VAR*NUM_PARTICLES;i++){
//				double rnd_temp = norm(urbg);
		double rnd_temp;
		randn(&rnd_temp,0,0);
		rnd_sigma[i] = rnd_temp;
	}

	for(int i=0; i< NUM_VAR*4;i++){
//				double rnd_temp = norm(urbg);
		double rnd_temp;
		randn(&rnd_temp,0,0);
		rnd_rk4[i] = rnd_temp;
	}
}
