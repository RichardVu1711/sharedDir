#include "ESPObj.h"
ESP_PF::ESP_PF(int* argc, char*** argv){
	
	 if (argc[0] != 2) {
        std::cout << "Usage: " << argv[0][0] << " <XCLBIN File>" << std::endl;
//        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[0][1];
    cl_int err;
    cl::Context context;
    cl::Kernel krnl_vector_add;
    cl::CommandQueue q[N_SRC];
	cl::Program program;
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
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        
        for(int i=0; i < N_SRC;i++){
        OCL_CHECK(err, q[i] = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
        	std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        }

        program = cl::Program(context, {device}, bins, nullptr, &err);
       
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }


    for(int i=0; i < N_SRC;i++){
        esp_control.q[i] = q[i];
    }
    esp_control.program = program;
    // esp_control.device = device;
}
