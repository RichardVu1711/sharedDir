#include "srcObj.h"
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

// conventionally
// obs_path provides path to obs data
// state and pxx will be in the same path
// but with different names as: Pxx_in and state_in}
// initialise the esp pf with data path
srcObj::srcObj(std::string obs_path,
				cl::Context& context,
				cl::CommandQueue& q){
	std::string state_path = obs_path;
	std::string pxx_path = obs_path;
	size_t lastSlashPos = obs_path.find_last_of('/');
	if (lastSlashPos != std::string::npos) {
		// replace obsValX to state_in.csv and pxx_in.csv
		state_path.replace(lastSlashPos + 1, std::string::npos, "state_in.csv");
		pxx_path.replace(lastSlashPos + 1, std::string::npos, "pxx_in.csv");
	}
//	convert_FP(read_csvMulLine(obs_path,0, 0+N_OBS, 10),
//								obs, 1, N_OBS*10, -1);
//	convert_FP(read_csvMulLine(state_path,0, 0+NUM_VAR, 1),
//								state, 1, NUM_VAR, -1);
//	convert_FP(read_csvMulLine(pxx_path,0, 0+NUM_VAR, NUM_VAR),
//								pxx, 1, NUM_VAR*NUM_VAR, -1);
	cout << "state -- Pxx\n";
	for(int i=0; i < 13;i++)
	{
		cout << state[i] << " -- " << pxx[i*13 + i] << "\n";
	}
	// create a link between ptr and openCL buffer with the given size and mode
	// for the sigmaComp Kernel Data
//	buffLink(&smpl_phase.espCrtInfo.statePro,size_state,context,q,RBUF,PL);
//	buffLink(&smpl_phase.espCrtInfo.sigMat,size_large,context,q,RBUF,PL);
//	buffLink(&smpl_phase.espCrtInfo.prtcls,size_large,context,q,WBUF,PL);

	// verify size debug
	cout << "smpl_phase: " << sizeof(smpl_info) << "\n";
	cout << "ptrBuff: " << sizeof(ptrBuff) << "\n";

//	smpl_phase = (smpl_info*) malloc(1*(sizeof(smpl_info)));
	buffLink(&smpl_phase.sigmaInfo.pxxSqrt,size_state,context,q,RBUF,PL);
	buffLink(&smpl_phase.sigmaInfo.rndIn,size_large,context,q,RBUF,PL);
	buffLink(&smpl_phase.sigmaInfo.sigMat,size_large,context,q,WBUF,PL);

	// dynamic memory allocation for rk4 as it is PS allocated
	buffLink(&smpl_phase.rk4Info.stateIn,size_state,context,q,RBUF,PS);
	buffLink(&smpl_phase.rk4Info.statePro,size_state,context,q,WBUF,PS);
//
//	// for the ESPCrtParticles kernel Data
	buffLink(&smpl_phase.espCrtInfo.statePro,size_state,context,q,RBUF,PL);
	buffLink(&smpl_phase.espCrtInfo.sigMat,size_large,context,q,RBUF,PL);
	buffLink(&smpl_phase.espCrtInfo.prtcls,size_large,context,q,WBUF,PL);

	convert_FP(read_csvMulLine(obs_path,0, 0+N_OBS, 10),
								obs, 1, N_OBS*10, -1);
	convert_FP(read_csvMulLine(state_path,0, 0+NUM_VAR, 1),
								state, 1, NUM_VAR, -1);
	convert_FP(read_csvMulLine(pxx_path,0, 0+NUM_VAR, NUM_VAR),
								pxx, 1, NUM_VAR*NUM_VAR, -1);

}
//  create buff:
//	PS: dynamic memory allocation the ptr
//	PL: create Opencl Buffer and linked it to the ptr
void srcObj::buffLink(ptrBuff* buffer,size_t in_size,
					cl::Context& context,
					cl::CommandQueue& q,
					rw_mode io_mode, PSPL alloc){
	cl_int err;
	fixed_type* ptr;
//	buffer[0].size = in_size;
	buffer[0].mode = io_mode;

	// if PS
	if(alloc == PS){
		buffer[0].ptr=(fixed_type*) malloc(in_size);
		buffer[0].allo_mode = PS;
	}
	else{  // PL => create OpenCL buffer + buff link
		buffer[0].allo_mode = PL;
		if(io_mode == RBUF){
			cout << "Buffer creation for read \n";
			OCL_CHECK(err, buffer[0].buf = cl::Buffer(context, CL_MEM_READ_ONLY, in_size, NULL, &err));
			OCL_CHECK(err, buffer[0].ptr = (fixed_type*)q.enqueueMapBuffer ( buffer[0].buf, CL_TRUE, CL_MAP_WRITE , 0, in_size, NULL, NULL, &err));
		}
		else{
			cout << "Buffer creation for write\n";
			OCL_CHECK(err, buffer[0].buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, in_size, NULL, &err));
		    OCL_CHECK(err, buffer[0].ptr = (fixed_type*)q.enqueueMapBuffer (buffer[0].buf, CL_TRUE, CL_MAP_READ , 0, in_size, NULL, NULL, &err));
		}
	}
}

// todo 22/12/2023: create buff:
//	PS: dynamic memory allocation the ptr
//	PL: create Opencl Buffer and linked it to the ptr
