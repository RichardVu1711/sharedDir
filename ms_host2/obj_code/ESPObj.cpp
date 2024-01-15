#include "ESPObj.h"


size_t size_Mat = 1 * sizeof(Mat);
size_t size_Mat_S = 1 * sizeof(Mat_S);
size_t size_wt = NUM_PARTICLES*sizeof(fixed_type);
size_t size_rndrk4 = 4*NUM_VAR*sizeof(fixed_type);

size_t size_pxx = NUM_VAR*NUM_VAR*sizeof(fixed_type);
size_t size_large = NUM_PARTICLES*NUM_VAR*sizeof(fixed_type);
//size_t size_prtcls = N_PL*NUM_VAR*sizeof(fixed_type);
size_t size_state = NUM_VAR*sizeof(fixed_type);

size_t size_msmt = 1*sizeof(msmt);
size_t size_Rmat = N_MEAS*sizeof(fixed_type);

size_t size_zDiff = NUM_PARTICLES*N_MEAS*sizeof(fixed_type);
size_t size_pzx = NUM_PARTICLES*N_MEAS*N_MEAS*sizeof(fixed_type);


void event_cb(cl_event event1, cl_int cmd_status, void* data) {
    cl_int err;
    cl::Event event(event1, true);
    cl_int status;
    OCL_CHECK(err, err = event.getInfo<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS,  &status));

    const char *status_str;
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
//        data[0] = (uint_8) 1;
        *(uint8_t*)data =1;
        break;
    }
//    printf("\nInterrupt occurred \n");
    fflush(stdout);
}

ESP_PF::ESP_PF(int* argc, char*** argv){
	
	 if (argc[0] != 2) {
        std::cout << "Usage: " << argv[0][0] << " <XCLBIN File>" << std::endl;
//        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[0][1];
    cl_int err;
//    cl::Context context;
    cl::CommandQueue q[N_SRC];
	cl::Program program;

    // OPENCL HOST CODE AREA START
    // get_xil_devices() is a utility API which will find the xilinx
    // platforms and will return list of devices connected to Xilinx platform
	// OPENCL HOST CODE AREA START
	// get_xil_devices() is a utility API which will find the xilinx
	// platforms and will return list of devices connected to Xilinx platform
	auto devices = xcl::get_xil_devices();
	// read_binary_file() is a utility API which will load the binaryFile
	// and will return the pointer to file buffer.
	auto fileBuf = xcl::read_binary_file(binaryFile);
	cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
	bool valid_device = false;
	for (unsigned int i = 0; i < devices.size(); i++) {
		auto device = devices[i];
		// Creating Context and Command Queue for selected Device
		OCL_CHECK(err, esp_control.context = cl::Context(device, nullptr, nullptr, nullptr, &err));
		for(int j=0; j < N_SRC;j++){
			OCL_CHECK(err, esp_control.q[j]= cl::CommandQueue(esp_control.context, device,  CL_QUEUE_PROFILING_ENABLE |
												CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
			std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
		}
		esp_control.program = cl::Program(esp_control.context, {device}, bins, nullptr, &err);
		if (err != CL_SUCCESS) {
			std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
		}
		else {
			valid_device = true;
		}
	}
	if (!valid_device) {
		std::cout << "Failed to program any device found, exit!\n";
//		exit(EXIT_FAILURE);
	}
		smpl_phase.sigmaInfo.allo_mode = PL;
		smpl_phase.rk4Info.allo_mode = PS;
		smpl_phase.espCrtInfo.allo_mode = PL;
		smpl_phase.mPxxInfo.allo_mode = PL;
		smpl_phase.axis2mmInfo.allo_mode = PL;
		this->irq_mode = SW_IRQ;
		if(irq_mode == SW_IRQ){
			this->smpl_phase.status.isCallBack= 0;
		}
		// configure state
		smpl_phase.status.current_state = 0;
		smpl_phase.status.next_state = 0;
		smpl_phase.status.previous_state = 0;
		smpl_phase.status.block_status = IDLE;
		smpl_phase.status.isDone = 0;
		// configure buffer
		buffLink(&smpl_phase.sigmaInfo.pxxSqrt,size_state,esp_control.context,esp_control.q[0],RBUF,PL);
		buffLink(&smpl_phase.sigmaInfo.rndIn,size_large,esp_control.context,esp_control.q[0],RBUF,PL);
		buffLink(&smpl_phase.sigmaInfo.sigMat,size_large,esp_control.context,esp_control.q[0],WBUF,PL);

		// dynamic memory allocation for rk4 as it is PS allocated
		buffLink(&smpl_phase.rk4Info.stateIn,size_state,esp_control.context,esp_control.q[0],RBUF,PS);
		buffLink(&smpl_phase.rk4Info.statePro,size_state,esp_control.context,esp_control.q[0],WBUF,PS);
		buffLink(&smpl_phase.rk4Info.rndrk4,size_rndrk4,esp_control.context,esp_control.q[0],WBUF,PS);

		// for the ESPCrtParticles kernel Data
		buffLink(&smpl_phase.espCrtInfo.statePro,size_state,esp_control.context,esp_control.q[0],RBUF,PL);
		buffLink(&smpl_phase.espCrtInfo.sigMat,size_large,esp_control.context,esp_control.q[0],RBUF,PL);
		buffLink(&smpl_phase.espCrtInfo.prtcls,size_large,esp_control.context,esp_control.q[0],WBUF,PL);

		// for the mPxx kernel Data
		buffLink(&smpl_phase.mPxxInfo.prtcls,size_large,esp_control.context,esp_control.q[0],RBUF,PL);
		buffLink(&smpl_phase.mPxxInfo.mPxx,size_pxx,esp_control.context,esp_control.q[0],WBUF,PL);
		// for the debugger
		buffLink(&smpl_phase.axis2mmInfo.prtclsIn,size_large,esp_control.context,esp_control.q[0],RBUF,PL);
		buffLink(&smpl_phase.axis2mmInfo.prtclsOut,size_large,esp_control.context,esp_control.q[0],WBUF,PL);

		//configure the kernel
		kernel_config(&smpl_phase.sigmaInfo.kSigma,esp_control.context, esp_control.program);
}

//  create buff:
//	PS: dynamic memory allocation the ptr
//	PL: create Opencl Buffer and linked it to the ptr
void ESP_PF::buffLink(ptrBuff* buffer,size_t in_size,
					cl::Context& context,
					cl::CommandQueue& q,
					rw_mode io_mode,  PSPL alloc){

	cl_int err;
	buffer[0].ptr=(fixed_type*) aligned_alloc(4096,in_size);
	if(alloc == PL){  // PL => create OpenCL buffer + buff link
		if(io_mode == RBUF){
			OCL_CHECK(err, buffer[0].buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
													in_size, buffer[0].ptr, &err));
		}
		else{
			OCL_CHECK(err, buffer[0].buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
													in_size, buffer[0].ptr, &err));
		}
	}
}

void ESP_PF::kernel_config(	cl::Kernel* kObj,
							cl::Context& context,
							cl::Program& program){
	cl_int err;

	// if there is a PL, then configure kernel. if the block is allocated on PS, then do nothing

	if(smpl_phase.sigmaInfo.allo_mode == PL){
		this->smpl_phase.sigmaInfo.kSigma = cl::Kernel(program,"sigmaComp", &err);
		OCL_CHECK(err, err = this->smpl_phase.sigmaInfo.kSigma.setArg(0,smpl_phase.sigmaInfo.pxxSqrt.buf));
		OCL_CHECK(err, err = this->smpl_phase.sigmaInfo.kSigma.setArg(2,smpl_phase.sigmaInfo.rndIn.buf)); // argument 1 is stream data

	}
	if(smpl_phase.rk4Info.allo_mode == PL){
		// as there is no rk4 kernel now, so this code will cause a fail
		this->smpl_phase.rk4Info.kRk4 = cl::Kernel(program,"rk4", &err);
		OCL_CHECK(err, err = this->smpl_phase.rk4Info.kRk4.setArg(0,smpl_phase.rk4Info.stateIn.buf));
		OCL_CHECK(err, err = this->smpl_phase.rk4Info.kRk4.setArg(1,smpl_phase.rk4Info.statePro.buf));
		OCL_CHECK(err, err = this->smpl_phase.rk4Info.kRk4.setArg(2,smpl_phase.rk4Info.rndrk4.buf));

	}
	if(smpl_phase.espCrtInfo.allo_mode == PL){
		this->smpl_phase.espCrtInfo.kCreate = cl::Kernel(program,"ESPCrtParticles", &err);
		OCL_CHECK(err, err = this->smpl_phase.espCrtInfo.kCreate.setArg(0,smpl_phase.espCrtInfo.statePro.buf));
	}
	if(smpl_phase.mPxxInfo.allo_mode == PL){
		this->smpl_phase.mPxxInfo.kmPxx = cl::Kernel(program,"mean_Pxx", &err);	// argument 0 is prtcls, stream interface
		OCL_CHECK(err, err = this->smpl_phase.mPxxInfo.kmPxx.setArg(1,smpl_phase.mPxxInfo.mPxx.buf));
	}
	if(smpl_phase.axis2mmInfo.allo_mode == PL){
		this->smpl_phase.axis2mmInfo.kaxis2mm = cl::Kernel(program,"axis2mm", &err);
//		OCL_CHECK(err, err = this->smpl_phase.espCrtInfo.kCreate.setArg(0,smpl_phase.espCrtInfo.statePro.buf)); // data is streamed into this kernel
		OCL_CHECK(err, err = this->smpl_phase.axis2mmInfo.kaxis2mm.setArg(1,smpl_phase.axis2mmInfo.prtclsOut.buf));

	}
}

// return 0 if the flag status is completed
// otherwise, return 0
int ESP_PF::getFlagInfo(){
		cl_int status;
		cl_int err;
		OCL_CHECK(err, err = smpl_phase.status.flag.getInfo<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS, &status));
		if(status == CL_COMPLETE)
			return 0;
		return 1;
}

// return 0 if finished
// other return 1
int ESP_PF::flagCheck_S(int qIdx){
	switch(irq_mode){
	case SEQ:
		getQueue(qIdx).finish();
		smpl_phase.status.isDone = 1;
		return 0;
		break;
	case POLL:
		smpl_phase.status.isDone = getFlagInfo();
		return getFlagInfo();

		break;
	case SW_IRQ:
		if(smpl_phase.status.isCallBack==0){
			this->set_callback(smpl_phase.status.flag);
		}
		return smpl_phase.status.isDone;
		break;
	default:
		return -1;
	}
	return 1;
}




void ESP_PF::set_callback(cl::Event event) {
    cl_int err;
	OCL_CHECK(err, err = event.setCallback(CL_COMPLETE, event_cb, (void*) &smpl_phase.status.isDone));
	this->smpl_phase.status.isCallBack = 1;

}


