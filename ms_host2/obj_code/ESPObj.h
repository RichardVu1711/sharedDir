#pragma once
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1


#define N_SRC 1
//#define N_PL 1024
//#define N_PORTION NUM_PARTICLES/N_PL

#include "../global_define/global_define.h"
#include "../global_define/GISmsmt_prcs.h"
#include "../global_define/mat_lib.h"
#include "../global_define/read_write_csv.h"
#include "srcObj.h"
#include <CL/cl2.hpp>


#include <iostream>

extern size_t size_Mat;
extern size_t size_Mat_S;
extern size_t size_wt;
extern size_t size_rndrk4;

extern size_t size_pxx;	// NUM_VAR*NUM_VAR -- 13 x 13
extern size_t size_large; // NUM_VAR*NUM_PARTICLES/N_PORTION -- 13 x 1024/N_PORTION
extern size_t size_prtcls; // NUM_VAR*NUM_PARTICLES
extern size_t size_state; // NUM_VAR * 1 -- 13x1

extern size_t size_msmt;
extern size_t size_Rmat;

extern size_t size_zDiff;
extern size_t size_pzx;

typedef enum rw_mode{
	RBUF,
	WBUF
} rw_mode;
typedef enum Bstatus{
	IDLE,
	EXEC,
	WAIT
} Bstatus;

typedef enum IRQflag{
	SEQ,
	POLL, 	// check the flag from event
	IRQ,
	SW_IRQ
} IRQflag;

typedef enum PSPL{
	PS,
	PL
} PSPL;

typedef struct rndCtrl{
	uint8_t isInit;
	int seed;
} rndCtrl;

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


typedef struct ptrBuff{
	cl::Buffer buf;
	fixed_type* ptr;
}ptrBuff;


typedef struct block_fsm{
	int next_state;
	int previous_state;
	int current_state;
	Bstatus block_status;
	uint8_t isDone;
	uint8_t isCallBack = 0;	// this value is used only for SW_IRQ
	cl::Event flag;
}block_fsm;
//kernel sigma parameter
typedef struct k_sigma{
	ptrBuff sigMat;
	ptrBuff pxxSqrt;
	ptrBuff rndIn;
	cl::Kernel kSigma;
	PSPL allo_mode;
} k_sigma;

typedef struct k_rk4{
	ptrBuff stateIn;
	ptrBuff statePro;
	ptrBuff rndrk4;
	cl::Kernel kRk4;
	PSPL allo_mode;
	// may want to put function pointer here for the future
} k_rk4;

typedef struct k_espCrt{
	ptrBuff sigMat;
	ptrBuff prtcls;
	ptrBuff statePro;
	cl::Kernel kCreate;
	PSPL allo_mode;
} k_espCrt;

typedef struct k_mPxx{
	ptrBuff mPxx;
	ptrBuff prtcls;
	cl::Kernel kmPxx;
	PSPL allo_mode;
} k_mPxx;

typedef struct k_axis2mm{
	ptrBuff prtclsIn;
	ptrBuff prtclsOut;
	cl::Kernel kaxis2mm;
	PSPL allo_mode;
} k_axis2mm;

//Sampling Data
typedef struct smpl_info{
	k_espCrt espCrtInfo;
	k_sigma sigmaInfo;
	k_rk4 rk4Info;	// this block is allocated under PS
	k_mPxx mPxxInfo;
	k_axis2mm axis2mmInfo;
	fixed_type* tmp_buf = NULL;	// tmp_ is a temp buffer to store particles data
	block_fsm status;
} smpl_info;

void event_cb(cl_event event1, cl_int cmd_status, void *data);

class ESP_PF{
public:

	queue esp_control;
	smpl_info smpl_phase;
	IRQflag irq_mode;
	ESP_PF(int* argc, char*** argv);
	void buffLink(ptrBuff* buffer,size_t in_size,
					cl::Context& context,
					cl::CommandQueue& q,
					rw_mode io_mode, PSPL alloc);
	void kernel_config(cl::Kernel* kObj,
						cl::Context& context,
						cl::Program& program);
	// return 1 if the flag status is completed
	// otherwise, return 0
	int getFlagInfo();
	cl::CommandQueue getQueue(int idx){
		return esp_control.q[idx];
	}
	PSPL getAlloMode_sigmaComp(){
		return smpl_phase.sigmaInfo.allo_mode;
	}
	PSPL getAlloMode_rk4(){
		return smpl_phase.rk4Info.allo_mode;
	}
	PSPL getAlloMode_ESPCrtParticles(){
		return smpl_phase.espCrtInfo.allo_mode;
	}
	int flagCheck_S(int qIdx);
	void isClearDoneSFlag(){
		//clear isDone and isCallBack
		smpl_phase.status.isDone =0;
		smpl_phase.status.isCallBack = 0;
		smpl_phase.status.flag = cl::Event();
	}
//	void event_cb(cl_event event1, cl_int cmd_status, void *data);
	void set_callback(cl::Event event);
};





