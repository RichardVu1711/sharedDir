#include "srcObj.h"


// conventionally
// obs_path provides path to obs data
// state and pxx will be in the same path
// but with different names as: Pxx_in and state_in}
// initialise the esp pf with data path
srcObj::srcObj(std::string obs_path,
				uint8_t idx,
				cl::Context& context,
				cl::CommandQueue& q){
	srcIdx = idx;
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
	for(int i=0; i < NUM_PARTICLES;i++){
		wt[i] = (fixed_type) 1.0/NUM_PARTICLES;
	}

	// initialise the current avg obs for outlier detectors
	for(int i=0; i < N_MEAS;i++){
		cAvg[i] = obs[i];
		nAvg[i] = 0;
	}

	// allocate memory for pzx and zDiff
//	pzx = (fixed_type*) malloc(size_pzx);
//	zDiff = (fixed_type* ) malloc(size_zDiff);
}
