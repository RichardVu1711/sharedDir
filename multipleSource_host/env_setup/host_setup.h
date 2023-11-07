#pragma once

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define Q_LEN 3

#include <CL/cl2.hpp>

#include "xcl2.hpp"
#include "experimental/xrt_profile.h"

#include "../global_define/global_define.h"
#include "../global_define/read_write_csv.h"
#include "../global_define/mat_lib.h"
#include "../random_generator/normrnd.h"
#include "../random_generator/randn.h"
#include "../resample_pf/resample_pf.h"
#include "../mvnpdf/mvnpdf_code.h"
#include "../calweights/Calweights.h"
#include "../rk4/rk4.h"
#include "../GISmsmt_prcs.h"

#include <pthread.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
static const std::string error_message =
    "Error: Result mismatch:\n"
    "i = %d CPU result = %d Device result = %d\n";


#define WBUF 1
#define RBUF 0

#define N_SRC 3

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

extern int S_status;
extern int C_status;
extern int R_status;

extern xrt::profile::user_range range;
extern xrt::profile::user_event events;
extern cl::Event status1;
extern cl::Event status2;
extern cl::Event status3;

extern int done;
extern pthread_t t;
enum state
{
    SAMP=1,
    CAL =2,
	UP =3,
	IDLE =0
};
typedef enum state state_t;
int device_setup(int argc, char* argv[],
				std::vector<cl::Device>& devices_lc,
			    cl::Device& device_lc,
			    cl::Context& context_lc,
			    cl::CommandQueue q_lc[Q_LEN],
			    cl::Program& program_lc);

int buf_link(int** ptr, cl::Buffer& buf, size_t in_size, int isWrite, int qIdx);

int block_S(int** pM_pxxIn, fixed_type pxx[NUM_VAR*NUM_VAR],cl::Buffer &bM_pxxIn,
			int** p_stateIn, fixed_type state[NUM_VAR], cl::Buffer &b_stateIn,
			int** p_sigMatIn, cl::Buffer &b_sigMatIn,
			int** p_sigMatOut, cl::Buffer &b_sigMatOut,
			int** p_rndIn, cl::Buffer &b_rndIn,
			int** p_prtclsOut, cl::Buffer &b_prtclsOut,
			cl::Kernel& kSigma,cl::Kernel& kCreate,
			state_t* s_state, int C_stt,
			int idx_s, cl::Event* status_S, cl::Event* status_S1);

int block_C(int** p_prtclsOut,int** p_prtclsIn, cl::Buffer &b_prtclsIn, fixed_type prtcls[NUM_VAR*NUM_PARTICLES],
			int** p_pxxOut, cl::Buffer &b_pxxOut,
			fixed_type obs_data[10],
			int** p_msmtIn, cl::Buffer &b_msmtIn,
			int** p_RmatIn, cl::Buffer &b_RmatIn,
			int** p_pxxIn, cl::Buffer &b_pxxIn,
			int** p_zDiffOut, cl::Buffer &b_zDiffOut,
			int** p_pzxOut, cl::Buffer &b_pzxOut,
			int step, double* N_eff,
			fixed_type wt[NUM_PARTICLES],
			cl::Kernel k_mPxx, cl::Kernel kCal,
			state_t* state,	int r_stt,
			int idx_s);

int block_R( fixed_type prtcls[NUM_VAR*NUM_PARTICLES],fixed_type wt[NUM_PARTICLES], double N_eff,
			int** p_prtclsIn, cl::Buffer& b_prtclsIn,
			int** p_wtIn, cl::Buffer& b_wtIn,
			int** p_stateOut, cl::Buffer& b_stateOut,
			int** p_pxxOut, cl::Buffer& b_pxxOut,
			int** p_stateIn, int** p_pxxIn,
			fixed_type state[NUM_VAR],
			fixed_type pxx[NUM_VAR*NUM_VAR],
			int step,
			cl::Kernel& kPFU,
			state_t* state_s,
			int idx_s);

void rng(fixed_type rnd_rk4[NUM_VAR],
		fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES]);

void* wait_thread(cl::CommandQueue* q);

#define OCL_CHECK(error, call)                                                                   \
    call;                                                                                        \
    if (error != CL_SUCCESS) {                                                                   \
        printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
        exit(EXIT_FAILURE);                                                                      \
    }


