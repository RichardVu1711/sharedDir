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
#include "../global_define/GISmsmt_prcs.h"

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

extern xrt::profile::user_range range;
extern xrt::profile::user_event events;

extern int done;
extern pthread_t t;
extern double N_eff;

extern fixed_type Grng_rk4[NUM_VAR*4];
extern fixed_type Grng_sigma[NUM_VAR*NUM_PARTICLES];
enum state
{
	IDLE = -1,
	INIT = 0,
    SAMP=1,
    CAL =2,
	UP =3
};
typedef enum state state_t;

enum samp_state{
	SWAIT = -1,
	SINIT = 0,
	P1 = 1,
	P2 = 2,
	P3 = 3,
	P4 = 4
};
typedef enum samp_state samp_state_t;

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
			int** p_prtclsOut, cl::Buffer &b_prtclsOut, fixed_type prtcls[NUM_VAR*NUM_PARTICLES], fixed_type prtclsTmp[NUM_VAR*NUM_PARTICLES],
			int** p_prtclsIn, cl::Buffer &b_prtclsIn,
			int** p_pxxOut, cl::Buffer &b_pxxOut,
			cl::Kernel& kSigma, cl::Kernel& kCreate,cl::Kernel& k_mPxx,
			state_t* nstate, samp_state_t* pbS, samp_state_t* nbS,
			int* Sinit, int S_status, int C_stt,
			int idx_s, cl::Event* done_S);

int block_C(fixed_type prtcls[NUM_VAR*NUM_PARTICLES],
			int** p_pxxOut, int** p_pxxIn, cl::Buffer &b_pxxIn,
			int** p_msmtIn, cl::Buffer &b_msmtIn, int* n_meas,
			int** p_RmatIn, cl::Buffer &b_RmatIn,
			fixed_type obs_data[10],
			int** p_zDiffOut, cl::Buffer &b_zDiffOut,
			int** p_pzxOut, cl::Buffer &b_pzxOut,
			int step,
			cl::Kernel& kCal,
			state_t* nstate, samp_state_t* pbC, samp_state_t* nbC,
			int* Cinit, int C_status, int r_stt, int* s_stt,
			int idx_s, cl::Event* done_C);

int block_R(fixed_type prtcls[NUM_VAR*NUM_PARTICLES],fixed_type wt[NUM_PARTICLES],
			int n_meas,
			int** p_zDiff,	int** p_pzx,
			int** p_prtclsIn, cl::Buffer& b_prtclsIn,
			int** p_wtIn, cl::Buffer& b_wtIn,
			int** p_stateOut, cl::Buffer& b_stateOut,
			int** p_pxxOut, cl::Buffer& b_pxxOut,
			fixed_type state[NUM_VAR],
			fixed_type pxx[NUM_VAR*NUM_VAR],
			int step,
			cl::Kernel& kPFU,
			state_t* nstate, state_t* pstate,samp_state_t* pbR, samp_state_t* nbR,
			int* Rinit, int R_status, int* c_stt,
			int idx_s, cl::Event* done_R);

void rng(fixed_type rnd_rk4[NUM_VAR],
		fixed_type rnd_sigma[NUM_VAR*NUM_PARTICLES]);

void* wait_thread(cl::CommandQueue* q);
void wait_for_enter() ;
#define OCL_CHECK(error, call)                                                                   \
    call;                                                                                        \
    if (error != CL_SUCCESS) {                                                                   \
        printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__, __LINE__, error); \
        exit(EXIT_FAILURE);                                                                      \
    }


