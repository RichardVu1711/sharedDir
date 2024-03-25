#pragma once
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1


#define N_SRC 1
//#define N_PL 1024
//#define N_PORTION NUM_PARTICLES/N_PL

#include "../lib/global_define.h"
#include "../lib/GISmsmt_prcs.h"
#include "../lib/mat_lib.h"
#include "../lib/read_write_csv.h"
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

extern size_t size_pzx_1; // size pzx for 1 particles
extern size_t size_zDiff_1; // size zDiff for 1 particles
extern size_t size_fp;

typedef enum rw_mode{
	RBUF,
	WBUF
} rw_mode;
typedef enum Bstatus{
	READY,
	EXEC,
	WAIT,
	WAIT2
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

typedef struct queue_ctl{
	std::vector<cl::Device> devices;
	cl::Device device;
	cl::Context context;
	cl::Program program;
	std::vector<cl::Platform> platforms;
	cl::CommandQueue q[N_SRC];
	int iterations;
	int MCrun;
} queue_ctl;

typedef struct ptrBuff{
	cl::Buffer buf;
	fixed_type* ptr;
}ptrBuff;


typedef struct block_fsm{
	int next_state;
	int previous_state;
	int current_state;
	Bstatus block_status;
	uint8_t isDone =0;
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
	PSPL allo_mode = PS;
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
	PSPL allo_mode = PS;
} k_axis2mm;

typedef struct k_mm2axis{
	ptrBuff prtclsIn;
	ptrBuff prtclsOut;
	cl::Kernel kmm2axis;
	PSPL allo_mode = PS;
} k_mm2axis;

typedef struct k_calW{
	ptrBuff prtcls;
	ptrBuff msmtinfo;
	ptrBuff R_Mat;
	ptrBuff Pxx_;
	ptrBuff pzx;
	ptrBuff zDiff;
	cl::Kernel kCalW;
	int index=0;
	PSPL allo_mode = PS;
} k_calW;

typedef struct k_mvnpdf{
	ptrBuff zDiff;
	ptrBuff pzx;
	ptrBuff p_val;
	cl::Kernel kMvnpdf;
	PSPL allo_mode = PS;
} k_mvnpdf;

typedef struct k_rsmpl{
	ptrBuff prtclsIn;
	ptrBuff wtIn;
	ptrBuff prtclsOut;
	ptrBuff wtOut;
	fixed_type r;
	cl::Kernel kRSMPL;
	PSPL allo_mode = PS;
} k_rsmpl;

typedef struct k_PFU{
	ptrBuff prtcls;
	ptrBuff wt;
	ptrBuff stateOut;
	ptrBuff pxxOut;
	cl::Kernel kPFU;
	PSPL allo_mode = PS;
} k_PFU;
//Sampling Data
typedef struct smpl_info{
	k_espCrt espCrtInfo;
	k_sigma sigmaInfo;
	k_rk4 rk4Info;	// this block is allocated under PS
	k_mPxx mPxxInfo;
	k_axis2mm axis2mmInfo;
	k_mm2axis mm2axisInfo;
	fixed_type* tmp_buf = NULL;	// tmp_ is a temp buffer to store particles data
	block_fsm status;
} smpl_info;

typedef struct calW_info{
	k_calW calWInfo;
	block_fsm status;
} calW_info;

typedef struct rsmp_info{
	k_mvnpdf mvnpdfInfo;
	k_rsmpl rsmplInfo;
	k_PFU PFUInfo;
	block_fsm status;
} rsmp_info;

void event_cb(cl_event event1, cl_int cmd_status, void *data);


class ESP_PF{
private:
	int buff_free(ptrBuff* buffer,PSPL alloc);
public:
	queue_ctl esp_control;
	smpl_info smpl_phase;
	calW_info calW_phase;
	rsmp_info rsmp_phase;
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
	int getFlagInfo(int block);
	cl::CommandQueue getQueue(int idx){
		return esp_control.q[idx];
	}
	// allocation mode for kernels
	PSPL getAlloMode_sigmaComp(){
		return smpl_phase.sigmaInfo.allo_mode;
	}
	PSPL getAlloMode_rk4(){
		return smpl_phase.rk4Info.allo_mode;
	}
	PSPL getAlloMode_ESPCrtParticles(){
		return smpl_phase.espCrtInfo.allo_mode;
	}
	PSPL getAlloMode_calW(){
		return calW_phase.calWInfo.allo_mode;
	}
	PSPL getAlloMode_PFU(){
		return rsmp_phase.PFUInfo.allo_mode;
	}
	PSPL getAlloMode_mvnpdf(){
		return rsmp_phase.mvnpdfInfo.allo_mode;
	}
	PSPL getAlloMode_rsmpl(){
		return rsmp_phase.rsmplInfo.allo_mode;
	}
	// working with block status
	Bstatus getBlockStatus_S(){
		return smpl_phase.status.block_status;
	}
	Bstatus getBlockStatus_C(){
		return calW_phase.status.block_status;
	}
	Bstatus getBlockStatus_R(){
		return rsmp_phase.status.block_status;
	}

	void setBlockStatus_S(Bstatus in_status){
		smpl_phase.status.block_status = in_status;
	}
	void setBlockStatus_C(Bstatus in_status){
		calW_phase.status.block_status = in_status;
	}
	void setBlockStatus_R(Bstatus in_status){
		rsmp_phase.status.block_status = in_status;
	}
	// working with class
	void isClearDoneSFlag(){
		//clear isDone and isCallBack
		smpl_phase.status.isDone =0;
		smpl_phase.status.isCallBack = 0;
		smpl_phase.status.flag = cl::Event();
	}
	void isClearDoneCFlag(){
		//clear isDone and isCallBack
		calW_phase.status.isDone =0;
		calW_phase.status.isCallBack = 0;
		calW_phase.status.flag = cl::Event();
	}
	void isClearDoneRFlag(){
		//clear isDone and isCallBack
		rsmp_phase.status.isDone =0;
		rsmp_phase.status.isCallBack = 0;
		rsmp_phase.status.flag = cl::Event();
	}
	int flagCheck_S(int qIdx);
	int flagCheck_C(int qIdx);
	int flagCheck_R(int qIdx);



	void status_init(block_fsm* in_status);
	void set_callback(cl::Event event, uint8_t* is_done);
	int releaseBuff();
};

int kernel_exec(
		ESP_PF* imp,uint8_t qIdx,
		cl::Kernel& kernel,
		vector<cl::Memory> memIn,
		vector<cl::Memory> memOut,
		std::vector<cl::Event> kernel_lst,
		cl::Event* data_events,
		cl::Event* exec_events) ;
int kernel_exec(
		ESP_PF* imp,uint8_t qIdx,
		cl::Kernel& kernel,
		vector<cl::Memory> memInOut,
		std::vector<cl::Event> kernel_lst,
		cl::Event* exec_events,
		uint8_t io_dir);


