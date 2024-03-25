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

