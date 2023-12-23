#pragma once
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1


#define N_SRC 1
#define N_OBS 52

#include "../global_define/global_define.h"
#include "../global_define/GISmsmt_prcs.h"
#include "../global_define/mat_lib.h"
#include "../global_define/read_write_csv.h"

#include <CL/cl2.hpp>

#include "xcl2.hpp"
#include "experimental/xrt_profile.h"
#include <iostream>

typedef struct queue{
	std::vector<cl::Device> devices;
	cl::Device device;
	cl::Context context;
	cl::Program program;
	std::vector<cl::Platform> platforms;
	cl::CommandQueue q[N_SRC];
	int iterations;
	int MCrun;
} queue;


class ESP_PF{
public:
	ESP_PF(int* argc, char*** argv);
	queue esp_control;
	cl::Kernel kPFU;
	cl::Kernel kCal;
	cl::Kernel kMPxx;
	cl::Kernel kSigma;
	cl::Kernel kCreate;
};





