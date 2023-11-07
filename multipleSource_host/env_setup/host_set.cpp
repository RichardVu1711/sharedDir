#include "host_setup.h"

void wait_for_enter() {
    std::cout << "Pause the program ...." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

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

fixed_type Grng_rk4[NUM_VAR*4];
fixed_type Grng_sigma[NUM_VAR*NUM_PARTICLES];
pthread_t t;

double N_eff =0;
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
			int** p_prtclsOut, cl::Buffer &b_prtclsOut, fixed_type prtcls[NUM_VAR*NUM_PARTICLES], fixed_type prtclsTmp[NUM_VAR*NUM_PARTICLES],
			int** p_prtclsIn, cl::Buffer &b_prtclsIn,
			int** p_pxxOut, cl::Buffer &b_pxxOut,
			cl::Kernel& kSigma, cl::Kernel& kCreate,cl::Kernel& k_mPxx,
			state_t* nstate, samp_state_t* pbS, samp_state_t* nbS,
			int* Sinit, int S_status, int C_stt,
			int idx_s, cl::Event* done_S)
{

	// copy input for sigmaComp block
	// BUG status, stt_1, or stt_2 won't be saved
	cl_int err;
	fixed_type state_pro[NUM_VAR];	//essential variable for rk4
	fixed_type mPxx[NUM_VAR*NUM_VAR];	//essential variable for rk4

	int tracking = 0;	// keep tracking states of block_S
								// I used this variable as
	switch(pbS[0])
	{
	case SINIT:{
		tracking =0;
		//copy data required for this block
		memcpy(&pM_pxxIn[0][0],pxx,size_pxx);
		string  pxx_dir = "/mnt/result/pxx"+to_string(idx_s) +".csv";
		string  state_dir = "/mnt/result/state"+to_string(idx_s) +".csv";

		write_csv(pxx_dir,convert_double(pxx,1,13*13,-1),13,13);
		write_csv(state_dir,convert_double(state,1,13*1,-1),1,13);

		nbS[0] = P1;
		Sinit[0] = 1;
		// put here as a safe state.
	}
	break;
	case P1:{
		tracking =1;
		fixed_type rnd[NUM_VAR*NUM_PARTICLES];
		fixed_type rnd_rk4[4*NUM_VAR];
		rng(rnd_rk4,rnd);
		memcpy(&p_rndIn[0][0],rnd,size_large);

		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({bM_pxxIn,b_rndIn},0/* 0 means from host*/));
		OCL_CHECK(err, err = q[idx_s].enqueueTask(kSigma));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_sigMatOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,done_S));
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
		memcpy(p_stateIn[0],state_pro,size_state);


		// check if KSigma is finished
		cl_int stt;
		OCL_CHECK(err, err = done_S[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &stt));
		if(stt == CL_COMPLETE)	nbS[0] = P2;
		else nbS[0] = SWAIT;	// wait for KSigma Finished.
	}
	break;
	case P2:{
		tracking =2;

		// Copy input for ESPCrtParticles
		memcpy(p_sigMatIn[0],p_sigMatOut[0],size_large);

		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_stateIn,b_sigMatIn},0/* 0 means from host*/));
		OCL_CHECK(err, err = q[idx_s].enqueueTask(kCreate));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,done_S));

		memcpy(prtcls,p_prtclsOut[0],size_large);
		cl_int pdone;
		OCL_CHECK(err, err = done_S[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pdone));
		if(pdone == CL_COMPLETE)	nbS[0] = P3;
		else nbS[0] = SWAIT;	// wait for KSigma Finished.
	}
	break;
	case P3:{
		tracking =3;
	    OCL_CHECK(err, cl::Buffer b_prtclsTemp(context, CL_MEM_READ_ONLY, size_large, NULL, &err));
	    int *p_prtclsTemp;
	    OCL_CHECK(err, err = k_mPxx.setArg(0,b_prtclsTemp));
		buf_link(&p_prtclsTemp,b_prtclsTemp,size_large,WBUF,1);

		memcpy(p_prtclsTemp,p_prtclsOut[0],size_large);

		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsTemp},0/* 0 means from host*/));
		OCL_CHECK(err, err = q[idx_s].enqueueTask(k_mPxx));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_pxxOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,done_S));

		cl_int pdone;
		OCL_CHECK(err, err = done_S[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pdone));
		if(pdone == CL_COMPLETE)	nbS[0] = P4;
		else nbS[0] = SWAIT;	// wait for KSigma Finished.

	}
	break;
	case P4:{
		// check if the Block C is ready, preparing for the tranfering.
		tracking =4;
		if(C_stt == 0){
			nstate[0] = CAL;
			Sinit[0] =0;
			fixed_type sigMat[NUM_VAR*NUM_PARTICLES];

			// NOTE: recording data is for debugging purpose, need to turn it to improve the overall performance !!
			memcpy(sigMat,&p_sigMatOut[0][0],size_large);
			memcpy(state_pro,&p_stateIn[0][0],size_state);
			memcpy(prtcls,&p_prtclsOut[0][0],size_large);
			memcpy(prtclsTmp,&p_prtclsOut[0][0],size_large);
			memcpy(mPxx,&p_pxxOut[0][0],size_pxx);
			string  sigMat_dir = "/mnt/result/sigMat"+to_string(idx_s) +".csv";
			string  prtcls_dir = "/mnt/result/prtcls"+to_string(idx_s) +".csv";
			string  state_pro_dir = "/mnt/result/state_pro"+to_string(idx_s) +".csv";
			string  mPxx_dir = "/mnt/result/mPxx"+to_string(idx_s) +".csv";

			write_csv(sigMat_dir,convert_double(sigMat,1,13*1024,-1),13,1024);
			write_csv(state_pro_dir,convert_double(state_pro,1,13*1,-1),1,13);
			write_csv(prtcls_dir,convert_double(prtcls,1,13*1024,-1),13,1024);
			write_csv(mPxx_dir,convert_double(mPxx,1,13*13,-1),13,13);

		}
		else{nstate[0] = SAMP;}
		nbS[0] = SWAIT;	// wait for KSigma Finished.
	}
	break;
	case SWAIT:{
		tracking = S_status;
		if((tracking == 1) || (tracking == 2) || (tracking == 3) )
		{
			cl_int pdone;
			OCL_CHECK(err, err = done_S[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pdone));
			if(pdone == CL_COMPLETE){
				if(tracking == 1){
					nbS[0] = P2;
				}
				else if(tracking ==  2){
					nbS[0] = P3;
				}
				else if(tracking ==  3){
					nbS[0] = P4;
				}
				done_S[0] = cl::Event();

			}
			// if not done, then keep sleeping
			else{
				nbS[0] = SWAIT;
			}
		}
		else if(tracking == 0)
		{
			nbS[0] = SINIT;
		}
		else if(tracking == 4)
		{
			nbS[0] = P4;
		}
		else
		{
			cout << "\n Some Weird states at SAMP state, status = " << tracking << "\n";
			return -1;
		}
	}
	break;
	}
	pbS[0] = nbS[0];
	return tracking;

}

int block_C(fixed_type prtcls[NUM_VAR*NUM_PARTICLES],
			int** p_pxxOut, int** p_pxxIn, cl::Buffer &b_pxxIn,
			int** p_msmtIn, cl::Buffer &b_msmtIn, int* n_meas,
			int** p_RmatIn, cl::Buffer &b_RmatIn,
			fixed_type obs_data[10],
			int** p_zDiffOut, cl::Buffer &b_zDiffOut,
			int** p_pzxOut, cl::Buffer &b_pzxOut,
			int step, cl::Kernel& kCal,
			state_t* nstate, samp_state_t* pbC, samp_state_t* nbC,
			int* Cinit, int C_status, int r_stt, int* s_stt,
			int idx_s, cl::Event* done_C)
{

	cl_int err;
	msmt msmtinfo;
	Mat_S Rmat;
	fixed_type R [N_MEAS];
	fixed_type zDiff[NUM_PARTICLES*N_MEAS];
	fixed_type pzx[NUM_PARTICLES*N_MEAS*N_MEAS];

	int tracking =0;
	switch(pbC[0])
	{
	case INIT:{
		msmtinfo = msmt_prcs(obs_data);
		Rmat = R_cal(&msmtinfo);

		for(int i=0; i < N_MEAS;i++)
		{
			R[i] = Rmat.entries[i*NUM_VAR+i];
		}
		n_meas[0] = msmtinfo.n_aoa + msmtinfo.n_tdoa;
		memcpy(p_msmtIn[0],&msmtinfo,size_msmt);
		memcpy(p_RmatIn[0],R,size_Rmat);
		memcpy(p_pxxIn[0],p_pxxOut[0],size_pxx);

		// For debugging Purpose
//		fixed_type pxx[169];
//		memcpy(pxx,p_pxxOut[0],size_pxx);

		tracking =0;
		Cinit[0] = 1;
		nbC[0] = P1;
		// free block S
		s_stt[0] = 0;
	}
	break;
	case P1:{
		tracking =1;

		// copy new particles before it is destroyed by the previous S block.
		OCL_CHECK(err, cl::Buffer b_prtclsTemp(context, CL_MEM_READ_ONLY, size_large, NULL, &err));
		int *p_prtclsTemp;
		buf_link(&p_prtclsTemp,b_prtclsTemp,size_large,WBUF,1);
		memcpy(p_prtclsTemp,prtcls,size_large);

		OCL_CHECK(err, err = kCal.setArg(3,step));
	    OCL_CHECK(err, err = kCal.setArg(0,b_prtclsTemp));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsTemp,b_msmtIn,b_RmatIn,b_pxxIn},0/* 0 means from host*/));
		OCL_CHECK(err, err = q[idx_s].enqueueTask(kCal));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_zDiffOut,b_pzxOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,done_C));

		cl_int pdone;
		OCL_CHECK(err, err = done_C[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pdone));
		if(pdone == CL_COMPLETE)	nbC[0] = P2;
		else nbC[0] = SWAIT;	// wait for KSigma Finished.

	}
	break;
	case P2:{
		tracking =2;
		if(r_stt == 0){
			// after moving to new block.
			// C tracking is still != 0;
			// This ensures that block would be operated if it is not ready yet.
			nstate[0] = UP;
			Cinit[0] = 0;
			// check functionality
			memcpy(zDiff,p_zDiffOut[0],size_zDiff);
			memcpy(pzx,p_pzxOut[0],size_pzx);
			string  zDiff_dir = "/mnt/result/zDiff"+to_string(idx_s) +".csv";
			string  pzx_dir = "/mnt/result/pzx"+to_string(idx_s) +".csv";

			write_csv(zDiff_dir,convert_double(zDiff,1,6*1024,-1),1024,6);
			write_csv(pzx_dir,convert_double(pzx,1,36*1024,-1),6*1024,6);
		}
		else{
			nstate[0] = CAL;
		}
		nbC[0] = SWAIT;
	}
	break;
	case SWAIT:{
		tracking =C_status;
		if(tracking == 1){
			//check if the PL is finished.
			cl_int pdone;
			OCL_CHECK(err, err = done_C[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pdone));
			if(pdone == CL_COMPLETE){
				if(tracking == 1){
					//The PL is finished, let move to next phase
					nbC[0] = P2;
				}
				// empty this flag as a safety procedure (can't not read if there is no PL run).
				done_C[0] = cl::Event();
			}
			else{
				// next block is not finished => move to the next block
				nbC[0] = SWAIT;
			}
		}
		else if(tracking == 0){
			nbC[0] = SINIT;

		}
		else if(tracking == 2){
			nbC[0] = P2;
		}
		else{
			cout << "\n Some Weird states at CAL state, status = " << tracking << "\n";
			return -1;
		}

	}
	break;
	default:
		cout << "Some Werid state at CAL, with pbC ==" << pbC << " \n";
		return -1023;
	break;
	}
	pbC[0] = nbC[0];
	return tracking;
}

int block_R(fixed_type prtcls[NUM_VAR*NUM_PARTICLES],fixed_type wt[NUM_PARTICLES],
			int n_meas,
			int** p_zDiff,	int** p_pzx,
			int** p_prtclsIn, cl::Buffer& b_prtclsIn,
			int** p_wtIn, cl::Buffer& b_wtIn,
			int** p_stateOut, cl::Buffer& b_stateOut,
			int** p_pxxOut, cl::Buffer& b_pxxOut,
			fixed_type state[NUM_VAR],
			fixed_type pxx[NUM_VAR*NUM_VAR],
			int step,
			cl::Kernel& kPFU,
			state_t* nstate, state_t* pstate,samp_state_t* pbR, samp_state_t* nbR,
			int* Rinit, int R_status, int* c_stt,
			int idx_s, cl::Event* done_R)
{
	fixed_type pzx[N_MEAS*N_MEAS*NUM_PARTICLES];
	fixed_type zDiff[1*N_MEAS*NUM_PARTICLES];
	cl_int err;

	fixed_type sum_fp =0;
	int tracking =0;
	int n_obs = n_meas;

	switch(pbR[0]){
	case SINIT:{
		memcpy(zDiff,p_zDiff[0],size_zDiff);
		memcpy(pzx,p_pzx[0],size_pzx);
//		cout << "zDiff" << zDiff[0] << ", " << zDiff[1] << "\n";
//		cout << "pzx:" << pzx[0] << ", " << pzx[35] << "\n";
//		wait_for_enter();
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
		N_eff = 1.0/re_sum;

		if(N_eff < NUM_PARTICLES*0.5 || N_eff > DBL_MAX*0.5){
			double rnd_temp;
			randn(&rnd_temp,0,0);
			fixed_type rnd_rsmp = rnd_temp;
			resamplePF_wrap(prtcls,wt,rnd_rsmp);
			memcpy(p_prtclsIn[0],prtcls,size_large);
			cout << "resample required\n";
		}
		memcpy(p_wtIn[0], wt,size_wt);
		memcpy(p_prtclsIn[0], prtcls, size_large);
		std::string wt_dir = "/mnt/result/wtOut" + to_string(idx_s) + ".csv";
		write_csv(wt_dir,convert_double(wt,1,1024,-1),1,1024);
		write_csv("/mnt/result/prtclsUOut.csv",convert_double(prtcls,1,13*1024,-1),13,1024);

		// free block C
		tracking =0;
		Rinit[0] = 1;
		nbR[0] = P1;
		c_stt[0] = 0;
	}
	break;

	case P1:{
		tracking =1;
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_prtclsIn,b_wtIn},0/* 0 means from host*/));
		OCL_CHECK(err, err = q[idx_s].enqueueTask(kPFU));
		OCL_CHECK(err, err = q[idx_s].enqueueMigrateMemObjects({b_stateOut,b_pxxOut},CL_MIGRATE_MEM_OBJECT_HOST,NULL,done_R));
		cl_int pdone;
		OCL_CHECK(err, err = done_R[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pdone));
		if(pdone == CL_COMPLETE)	nbR[0] = P2;
		else nbR[0] = SWAIT;	// wait for kPFU Finished.

	}
		break;
	case P2:{
		tracking =2;
		cout << "\n\ntracking = 2\n";
		// after moving to new block.
		// C tracking is still != 0;
		// This ensures that block would be operated if it is not ready yet.
		nstate[0] = IDLE;
		pstate[0] = IDLE;
		Rinit[0] = 0;
		q[idx_s].finish();
		memcpy(state,p_stateOut[0],size_state);
		memcpy(pxx,p_pxxOut[0],size_pxx);
		// check functionality
		std::string state_dir = "/mnt/result/stateOut" + to_string(idx_s) + ".csv";
		std::string pxx_dir = "/mnt/result/pxxOut" + to_string(idx_s) + ".csv";
		write_csv(state_dir,convert_double(state,1,13,-1),1,13);
		write_csv(pxx_dir,convert_double(pxx,13,13,-1),13,13);
		cout << "\nstate= " << state[0] << ", " << state[1] << "\t pxx =" << pxx[0] << ", " << pxx[14] << "\n";
		cout << "End of : " << step << " with Neff =  "<< N_eff <<".\n";
		nbR[0] = SWAIT;	// wait for KSigma Finished.
		tracking =0;
	}
		break;
	case SWAIT:{
		tracking =R_status;
		if(tracking == 1){
			cl_int pdone;
			OCL_CHECK(err, err = done_R[0].getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &pdone));
			if(pdone == CL_COMPLETE){
				nbR[0] = P2;
				done_R[0] = cl::Event();
			}
			else{
				nbR[0] = SWAIT;
			}
		}
		else if(tracking == 0){
			nbR[0] = SINIT;

		}
		else if(tracking == 2){
			nbR[0] = P2;
		}
		else{
			cout << "\n Some Weird states at UPD state, status = " << tracking << "\n";
			return -1;
		}
	}
	break;
	default:
		cout << "\n Some Weird states at UPD state, status = " << tracking << "\n";
		break;
		return -1023;
	}

	pbR[0] = nbR[0];
	return tracking;
}

void rng(fixed_type rnd_rk4[NUM_VAR],
		 fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES])
{
	for(int i=0; i< NUM_VAR*NUM_PARTICLES;i++){
//				double rnd_temp = norm(urbg);
//		double rnd_temp;
//		randn(&rnd_temp,0,0);
//		rnd_sigma[i] = rnd_temp;
		rnd_sigma[i] = Grng_sigma[i];

	}
	write_csv("/mnt/result/rngSigma.csv" ,convert_double(rnd_sigma,13,1024,-1),13,1024);


	for(int i=0; i< NUM_VAR*4;i++){
//				double rnd_temp = norm(urbg);
//		double rnd_temp;
//		randn(&rnd_temp,0,0);
//		rnd_rk4[i] = rnd_temp;
		rnd_rk4[i] = Grng_rk4[i];
	}
}
