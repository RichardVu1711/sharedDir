#pragma once

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define Q_LEN 1

#define OCL_CHECK(error, call)                                                                   \
    call;                                                                                        \
    if (error != CL_SUCCESS) {                                                                   \
        printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
        exit(EXIT_FAILURE);                                                                      \
    }
#include <CL/cl2.hpp>

#include "../lib/global_define.h"
#include "../lib/read_write_csv.h"
#include "../lib/mat_lib.h"
#include "../lib/GISmsmt_prcs.h"

#include <stdlib.h>
#include <fstream>
#include <iostream>
static const std::string error_message =
    "Error: Result mismatch:\n"
    "i = %d CPU result = %d Device result = %d\n";


#define WBUF 1
#define RBUF 0

extern std::vector<cl::Device> devices;
extern cl::Device device;
extern cl::Context context;
extern cl::Program program;
extern std::vector<cl::Platform> platforms;
extern cl::CommandQueue q[Q_LEN];

extern size_t size_Mat;
extern size_t size_Mat_S;
extern size_t size_wt;

extern size_t size_pxx;
extern size_t size_large;
extern size_t size_state;

extern size_t size_msmt;
extern size_t size_Rmat;

extern size_t size_zDiff;
extern size_t size_pzx;

int device_setup(int argc, char* argv[],
				std::vector<cl::Device>& devices_lc,
			    cl::Device& device_lc,
			    cl::Context& context_lc,
			    cl::CommandQueue q_lc[Q_LEN],
			    cl::Program& program_lc);

int buf_link(int** ptr, cl::Buffer& buf, size_t in_size, int isWrite, int qIdx);



