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

//    const char *status_str;
    switch (status) {
    case CL_QUEUED:
//        status_str = "Queued";
        break;
    case CL_SUBMITTED:
//        status_str = "Submitted";
        break;
    case CL_RUNNING:
//        status_str = "Executing";
        break;
    case CL_COMPLETE:
//        status_str = "Completed";
//        data[0] = (uint_8) 1;
        *(uint8_t*)data =1;
        break;
    }

//    printf("\nInterrupt occurred \n");
//    fflush(stdout);
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

	calW_phase.calWInfo.allo_mode = PL;

	rsmp_phase.mvnpdfInfo.allo_mode = PS;
	rsmp_phase.rsmplInfo.allo_mode = PS;
	rsmp_phase.PFUInfo.allo_mode = PL;
	// configure IRQ mode
	this->irq_mode = SW_IRQ;
	if(irq_mode == SW_IRQ){
		this->smpl_phase.status.isCallBack= 0;
	}
	// configure state
	status_init(&smpl_phase.status);
	status_init(&calW_phase.status);

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

	// for the calW kernel Data
	buffLink(&calW_phase.calWInfo.prtcls,size_large,esp_control.context,esp_control.q[0],RBUF,calW_phase.calWInfo.allo_mode);
	buffLink(&calW_phase.calWInfo.msmtinfo,size_msmt,esp_control.context,esp_control.q[0],RBUF,calW_phase.calWInfo.allo_mode);
	buffLink(&calW_phase.calWInfo.R_Mat,size_Rmat,esp_control.context,esp_control.q[0],RBUF,calW_phase.calWInfo.allo_mode);
	buffLink(&calW_phase.calWInfo.Pxx_,size_pxx,esp_control.context,esp_control.q[0],RBUF,calW_phase.calWInfo.allo_mode);
	buffLink(&calW_phase.calWInfo.pzx,size_pzx,esp_control.context,esp_control.q[0],WBUF,calW_phase.calWInfo.allo_mode);
	buffLink(&calW_phase.calWInfo.zDiff,size_zDiff,esp_control.context,esp_control.q[0],WBUF,calW_phase.calWInfo.allo_mode);

	// for the mvnpdf kernel Data
	buffLink(&rsmp_phase.mvnpdfInfo.zDiff,size_zDiff,esp_control.context,esp_control.q[0],RBUF,rsmp_phase.mvnpdfInfo.allo_mode);
	buffLink(&rsmp_phase.mvnpdfInfo.pzx,size_pzx,esp_control.context,esp_control.q[0],RBUF,rsmp_phase.mvnpdfInfo.allo_mode);
	buffLink(&rsmp_phase.mvnpdfInfo.p_val,size_wt,esp_control.context,esp_control.q[0],WBUF,rsmp_phase.mvnpdfInfo.allo_mode);

	// for the PFU kernel Data
	buffLink(&rsmp_phase.PFUInfo.prtcls,size_large,esp_control.context,esp_control.q[0],RBUF,rsmp_phase.PFUInfo.allo_mode);
	buffLink(&rsmp_phase.PFUInfo.wt,size_wt,esp_control.context,esp_control.q[0],RBUF,rsmp_phase.PFUInfo.allo_mode);
	buffLink(&rsmp_phase.PFUInfo.stateOut,size_state,esp_control.context,esp_control.q[0],WBUF,rsmp_phase.PFUInfo.allo_mode);
	buffLink(&rsmp_phase.PFUInfo.pxxOut,size_pxx,esp_control.context,esp_control.q[0],WBUF,rsmp_phase.PFUInfo.allo_mode);

	// for the rsmpl kernel Data
	buffLink(&rsmp_phase.rsmplInfo.prtclsIn,size_large,esp_control.context,esp_control.q[0],RBUF,rsmp_phase.rsmplInfo.allo_mode);
	buffLink(&rsmp_phase.rsmplInfo.wtIn,size_wt,esp_control.context,esp_control.q[0],RBUF,rsmp_phase.rsmplInfo.allo_mode);
	buffLink(&rsmp_phase.rsmplInfo.prtclsOut,size_large,esp_control.context,esp_control.q[0],WBUF,rsmp_phase.rsmplInfo.allo_mode);
	buffLink(&rsmp_phase.rsmplInfo.wtOut,size_wt,esp_control.context,esp_control.q[0],WBUF,rsmp_phase.rsmplInfo.allo_mode);

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
	if(calW_phase.calWInfo.allo_mode == PL){
		this->calW_phase.calWInfo.kCalW = cl::Kernel(program,"CalPzxZdiff", &err);
		OCL_CHECK(err,err = this->calW_phase.calWInfo.kCalW.setArg(0,calW_phase.calWInfo.prtcls.buf));
		OCL_CHECK(err,err = this->calW_phase.calWInfo.kCalW.setArg(1,calW_phase.calWInfo.msmtinfo.buf));
		OCL_CHECK(err,err = this->calW_phase.calWInfo.kCalW.setArg(2,calW_phase.calWInfo.R_Mat.buf));
		OCL_CHECK(err,err = this->calW_phase.calWInfo.kCalW.setArg(3,calW_phase.calWInfo.index));
		OCL_CHECK(err,err = this->calW_phase.calWInfo.kCalW.setArg(4,calW_phase.calWInfo.Pxx_.buf));
		OCL_CHECK(err,err = this->calW_phase.calWInfo.kCalW.setArg(5,calW_phase.calWInfo.zDiff.buf));
		OCL_CHECK(err,err = this->calW_phase.calWInfo.kCalW.setArg(6,calW_phase.calWInfo.pzx.buf));
	}
	if(rsmp_phase.mvnpdfInfo.allo_mode == PL){
		// need to create mvnpdfInfo
		this->rsmp_phase.mvnpdfInfo.kMvnpdf = cl::Kernel(program, "mvnpdf", &err);
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(0,rsmp_phase.mvnpdfInfo.zDiff.buf));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(1,rsmp_phase.mvnpdfInfo.pzx.buf));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(2,rsmp_phase.mvnpdfInfo.p_val.buf));
	}
	if(rsmp_phase.rsmplInfo.allo_mode == PL){
		this->rsmp_phase.mvnpdfInfo.kMvnpdf = cl::Kernel(program, "resample_pf", &err);
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(0,rsmp_phase.rsmplInfo.prtclsIn.buf));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(1,rsmp_phase.rsmplInfo.wtIn.buf));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(2,rsmp_phase.rsmplInfo.r));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(3,rsmp_phase.rsmplInfo.prtclsOut.buf));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(4,rsmp_phase.rsmplInfo.wtOut.buf));
	}
	if(rsmp_phase.PFUInfo.allo_mode == PL){
		this->rsmp_phase.PFUInfo.kPFU = cl::Kernel(program,"PFupdate",&err);
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(0,rsmp_phase.PFUInfo.prtcls.buf));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(1,rsmp_phase.PFUInfo.wt.buf));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(2,rsmp_phase.PFUInfo.stateOut.buf));
		OCL_CHECK(err,err = this->rsmp_phase.PFUInfo.kPFU.setArg(3,rsmp_phase.PFUInfo.pxxOut.buf));
	}

	// configure the flag status for S C and R blocks
	this->setBlockStatus_S(READY);
	this->setBlockStatus_C(READY);
	this->setBlockStatus_R(READY);
}

// return 0 if the flag status is completed
// otherwise, return 0
int ESP_PF::getFlagInfo(int block){
		cl_int status;
		cl_int err;
		switch(block){
			case 0:
				OCL_CHECK(err, err = smpl_phase.status.flag.getInfo<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS, &status));
				if(status == CL_COMPLETE)
					return 0;
				break;
			case 1:
				OCL_CHECK(err, err = calW_phase.status.flag.getInfo<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS, &status));
				if(status == CL_COMPLETE)
					return 0;
				break;
			case 2:
				OCL_CHECK(err, err = rsmp_phase.status.flag.getInfo<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS, &status));
				if(status == CL_COMPLETE)
					return 0;
				break;

		}
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
		smpl_phase.status.isDone = getFlagInfo(0);
		return (int) smpl_phase.status.isDone;

		break;
	case SW_IRQ:
		if(smpl_phase.status.isCallBack==0){
			this->set_callback(smpl_phase.status.flag,&smpl_phase.status.isDone);
			this->smpl_phase.status.isCallBack = 1;
		}
		return smpl_phase.status.isDone;
		break;
	default:
		return -1;
	}
	return 1;
}
int ESP_PF::flagCheck_C(int qIdx){
	if(getAlloMode_calW()==PL){
		switch(irq_mode){
		case SEQ:
			getQueue(qIdx).finish();
			calW_phase.status.isDone = 1;
			return 0;
			break;
		case POLL:
			calW_phase.status.isDone = getFlagInfo(1);
			return (int) calW_phase.status.isDone;
			break;
		case SW_IRQ:
			if(calW_phase.status.isCallBack==0){
				this->set_callback(calW_phase.status.flag,&calW_phase.status.isDone);
				this->calW_phase.status.isCallBack = 1;
			}
			return calW_phase.status.isDone;
			break;
		default:
			return -1;
		}
	}
	return calW_phase.status.isDone;
}

int ESP_PF::flagCheck_R(int qIdx){
	switch(irq_mode){
	case SEQ:
		getQueue(qIdx).finish();
		rsmp_phase.status.isDone = 1;
		return 0;
		break;
	case POLL:
		rsmp_phase.status.isDone = getFlagInfo(2);
		return (int) rsmp_phase.status.isDone;
		break;
	case SW_IRQ:
		if(rsmp_phase.status.isCallBack==0){
			this->set_callback(rsmp_phase.status.flag, &rsmp_phase.status.isDone);
			this->rsmp_phase.status.isCallBack = 1;
		}
		return rsmp_phase.status.isDone;
		break;
	default:
		return -1;
	}
	return rsmp_phase.status.isDone;
}
void ESP_PF::status_init(block_fsm* in_status){
	in_status->current_state = 0;
	in_status->next_state = 0;
	in_status->previous_state = 0;
	in_status->block_status = READY;
	in_status->isDone = 0;
}


void ESP_PF::set_callback(cl::Event event,uint8_t* is_done) {
    cl_int err;
	OCL_CHECK(err, err = event.setCallback(CL_COMPLETE, event_cb, (void*) is_done));
}
int ESP_PF::releaseBuff(){

	buff_free(&smpl_phase.sigmaInfo.pxxSqrt,smpl_phase.sigmaInfo.allo_mode);
	buff_free(&smpl_phase.sigmaInfo.rndIn,smpl_phase.sigmaInfo.allo_mode);
	buff_free(&smpl_phase.sigmaInfo.sigMat,smpl_phase.sigmaInfo.allo_mode);

	// dynamic memory allocation for rk4 as it is PS allocated
	buff_free(&smpl_phase.rk4Info.stateIn,smpl_phase.rk4Info.allo_mode);
	buff_free(&smpl_phase.rk4Info.statePro,smpl_phase.rk4Info.allo_mode);
	buff_free(&smpl_phase.rk4Info.rndrk4,smpl_phase.rk4Info.allo_mode);

	// for the ESPCrtParticles kernel Data
	buff_free(&smpl_phase.espCrtInfo.statePro,smpl_phase.espCrtInfo.allo_mode);
	buff_free(&smpl_phase.espCrtInfo.sigMat,smpl_phase.espCrtInfo.allo_mode);
	buff_free(&smpl_phase.espCrtInfo.prtcls,smpl_phase.espCrtInfo.allo_mode);

	// for the mPxx kernel Data
	buff_free(&smpl_phase.mPxxInfo.prtcls,smpl_phase.mPxxInfo.allo_mode);
	buff_free(&smpl_phase.mPxxInfo.mPxx,smpl_phase.mPxxInfo.allo_mode);
	// for the debugger
	buff_free(&smpl_phase.axis2mmInfo.prtclsIn,smpl_phase.axis2mmInfo.allo_mode);
	buff_free(&smpl_phase.axis2mmInfo.prtclsOut,smpl_phase.axis2mmInfo.allo_mode);

	buff_free(&calW_phase.calWInfo.prtcls,calW_phase.calWInfo.allo_mode);
	buff_free(&calW_phase.calWInfo.msmtinfo,calW_phase.calWInfo.allo_mode);
	buff_free(&calW_phase.calWInfo.Pxx_,calW_phase.calWInfo.allo_mode);
	buff_free(&calW_phase.calWInfo.R_Mat,calW_phase.calWInfo.allo_mode);
	buff_free(&calW_phase.calWInfo.pzx,calW_phase.calWInfo.allo_mode);
	buff_free(&calW_phase.calWInfo.zDiff,calW_phase.calWInfo.allo_mode);

	buff_free(&rsmp_phase.rsmplInfo.prtclsIn,rsmp_phase.rsmplInfo.allo_mode);
	buff_free(&rsmp_phase.rsmplInfo.wtIn,rsmp_phase.rsmplInfo.allo_mode);
	buff_free(&rsmp_phase.rsmplInfo.prtclsOut,rsmp_phase.rsmplInfo.allo_mode);
	buff_free(&rsmp_phase.rsmplInfo.wtOut,rsmp_phase.rsmplInfo.allo_mode);

	buff_free(&rsmp_phase.mvnpdfInfo.zDiff,rsmp_phase.mvnpdfInfo.allo_mode);
	buff_free(&rsmp_phase.mvnpdfInfo.pzx,rsmp_phase.mvnpdfInfo.allo_mode);
	buff_free(&rsmp_phase.mvnpdfInfo.p_val,rsmp_phase.mvnpdfInfo.allo_mode);

	buff_free(&rsmp_phase.PFUInfo.prtcls,rsmp_phase.PFUInfo.allo_mode);
	buff_free(&rsmp_phase.PFUInfo.wt,rsmp_phase.PFUInfo.allo_mode);
	buff_free(&rsmp_phase.PFUInfo.stateOut,rsmp_phase.PFUInfo.allo_mode);
	buff_free(&rsmp_phase.PFUInfo.pxxOut,rsmp_phase.PFUInfo.allo_mode);

//	this->getQueue(qIdx).clReleaseKernel(smpl_phase.sigmaInfo.kSigma);
//	clReleaseKernel(smpl_phase.espCrtInfo.kCreate);
//	clReleaseKernel(smpl_phase.mPxxInfo.kmPxx);
//	clReleaseKernel(smpl_phase.axis2mmInfo.kaxis2mm);
//	clReleaseKernel(calW_phase.calWInfo.kCalW);
//	clReleaseKernel(rsmp_phase.PFUInfo.KPFU);
//
//	clReleaseProgram(esp_control.program);
//	for(int i=0; i < N_SRC;i++)	clReleaseCommandQueue(esp_control.q[i]);
//	clReleaseContext(esp_control.context);
	return 0;
}
int ESP_PF::buff_free(ptrBuff* buffer,PSPL alloc){
	cl_int err;
	uint8_t qIdx = 0;

	if(alloc== PL){
		buffer->buf = cl::Buffer();
//			   OCL_CHECK(err, err = this->getQueue(qIdx).enqueueUnmapMemObject(buffer->buf,
//									buffer->ptr, // pointer returned by Map call
//									nullptr, nullptr));
	}
	free(buffer->ptr);
	buffer->ptr = nullptr;
	return 0;
}


// this function is used to execute kernel stay on the PL.
// data event is an event object used as ack when finish transfering data from PS to PL
// kernel_lst: is to store all events used to schedule the execution of events
int kernel_exec(
		ESP_PF* imp,uint8_t qIdx,
		cl::Kernel& kernel,
		vector<cl::Memory> &memIn,
		vector<cl::Memory> &memOut,
		std::vector<cl::Event> kernel_lst,
		cl::Event* data_events,
		cl::Event* exec_events){
	cl_int err;
	OCL_CHECK(err, err = imp->esp_control.q[qIdx].enqueueMigrateMemObjects(memIn,0, NULL,data_events));
	kernel_lst.push_back(data_events[0]);
	OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueNDRangeKernel(kernel,0,1,1,
																&kernel_lst,&exec_events[0]));
	kernel_lst.push_back(exec_events[0]);
	OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueMigrateMemObjects(memOut,CL_MIGRATE_MEM_OBJECT_HOST,
			&kernel_lst,NULL));
	return 0;
}


