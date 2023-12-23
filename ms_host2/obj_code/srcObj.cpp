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
srcObj::srcObj(std::string obs_path,cl::CommandQueue &q){
	std::string state_path = obs_path;
	std::string pxx_path = obs_path;
	size_t lastSlashPos = obs_path.find_last_of('/');
	if (lastSlashPos != std::string::npos) {
		// replace obsValX to state_in.csv and pxx_in.csv
		state_path.replace(lastSlashPos + 1, std::string::npos, "state_in.csv");
		pxx_path.replace(lastSlashPos + 1, std::string::npos, "pxx_in.csv");
	}


	convert_FP(read_csvMulLine(obs_path,0, 0+N_OBS, 10),
								obs, 1, N_OBS*10, -1);
	convert_FP(read_csvMulLine(state_path,0, 0+NUM_VAR, 1),
								state, 1, NUM_VAR, -1);
	convert_FP(read_csvMulLine(pxx_path,0, 0+NUM_VAR, NUM_VAR),
								pxx, 1, NUM_VAR*NUM_VAR, -1);

	// create a link between ptr and openCL buffer with the given size and mode
	// for the sigmaComp Kernel Data
	buffLink(smpl_phase.sigmaInfo.pxxSqrt,size_state,q,RBUF,PL);
	buffLink(smpl_phase.sigmaInfo.rndIn,size_large,q,RBUF,PL);
	buffLink(smpl_phase.sigmaInfo.sigMat,size_large,q,WBUF,PL);

	// dynamic memory allocation for rk4 as it is PS allocated
	buffLink(smpl_phase.rk4Info.stateIn,size_state,q,RBUF,PS);
	buffLink(smpl_phase.rk4Info.statePro,size_state,q,WBUF,PS);

	// for the ESPCrtParticles kernel Data
	buffLink(smpl_phase.espCrtInfo.statePro,size_state,q,RBUF,PL);
	buffLink(smpl_phase.espCrtInfo.sigMat,size_large,q,RBUF,PL);
	buffLink(smpl_phase.espCrtInfo.prtcls,size_large,q,WBUF,PL);

}
//  create buff:
//	PS: dynamic memory allocation the ptr
//	PL: create Opencl Buffer and linked it to the ptr
void srcObj::buffLink(ptrBuff buffer,size_t in_size,
				cl::CommandQueue& q,
				rw_mode io_mode, PSPL alloc){
	cl_int err;
	// if PS
	if(alloc == PS){
		buffer.ptr=(fixed_type*) malloc(in_size);
	}
	else{  // PL => create OpenCL buffer + buff link
		if(io_mode == RBUF){
		    OCL_CHECK(err, buffer.ptr = (fixed_type*)q.enqueueMapBuffer (buffer.buf , CL_TRUE , CL_MAP_WRITE , 0, in_size, NULL, NULL, &err));
		}
		else{
		    OCL_CHECK(err, buffer.ptr = (fixed_type*)q.enqueueMapBuffer (buffer.buf , CL_TRUE , CL_MAP_READ , 0, in_size, NULL, NULL, &err));
		}
	}
}

// todo 22/12/2023: create buff:
//	PS: dynamic memory allocation the ptr
//	PL: create Opencl Buffer and linked it to the ptr
