#include "srcObj.h"

// conventionally
// obs_path provides path to obs data
// state and pxx will be in the same path
// but with different names as: Pxx_in and state_in}


// initialise the esp pf with data path
srcObj::init_(std::string obs_path){
	std::filesystem::path originalPath(originalPathString);
	std::string state_path = originalPath.replace_filename("state_in.csv");
	std::string pxx_path = originalPath.replace_filename("pxx_in.csv");

	convert_FP(read_csvMulLine(obs_path,0, 0+N_OBS, 10),
								srcObj.obs, 1, N_OBS*10, -1);
	convert_FP(read_csvMulLine(obs_path,0, 0+NUM_VAR, 1),
								srcObj.state, 1, NUM_VAR, -1);
	convert_FP(read_csvMulLine(obs_path,0, 0+NUM_VAR, NUM_VAR),
								srcObj.pxx, 1, NUM_VAR*NUM_VAR, -1);
	
}