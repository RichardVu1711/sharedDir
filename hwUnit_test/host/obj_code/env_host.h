#pragma once
#include "ESPObj.h"
#include "srcObj.h"

#include "xcl2.hpp"
#include "experimental/xrt_profile.h"

#include "../random_generator/RNG_withSeed.h"
#include "../rk4/rk4.h"
#include "../resample_pf/resample_pf.h"
#include "../calW/mvnpdf_code.h"
#include "../sigmaComp.h"
#include "../ESPCrtParticles.h"
#include "../mean_pxx.h"
// debug macros
#define MO(function_call) \
    do { \
        int result = function_call; \
        if (result != 0) { \
            std::cerr << "Error in function: " << #function_call << std::endl \
                      << "Returned value: " << result << std::endl \
                      << "File: " << __FILE__ << std::endl \
                      << "Line: " << __LINE__ << std::endl \
                      << "Called from: " << __PRETTY_FUNCTION__ << std::endl; \
            return result; \
        } \
    } while (0)

#define PROBE_(func) \
    do { \
        printf("\nCalling %s file - %s() function - Line %d\n", __FILE__, #func, __LINE__); \
        func; \
    } while (0)


int smplPhase_execute(ESP_PF* imp, srcObj* srcX);
void wait_for_enter();
int rk4_execute(ESP_PF* imp, srcObj* srcX);
int ESPCrtParticles_execute(ESP_PF* imp, srcObj* srcX,
							std::vector<cl::Event> kernel_events,
							cl::Event* data_events,
							cl::Event* exec_events);
int sigma_execute(ESP_PF* imp, srcObj* srcX,
				std::vector<cl::Event> kernel_lst,
				cl::Event* data_event,
				cl::Event* exec_event);
int mPxx_execute(ESP_PF* imp, srcObj* srcX,
				std::vector<cl::Event> kernel_events,
				cl::Event* data_events,
				cl::Event* exec_events);

int calwPhase_execute(ESP_PF* imp, srcObj* srcX);
int calW_execute(ESP_PF* imp, srcObj* srcX);

int rsmpPhase_execute(ESP_PF* imp, srcObj* srcX);
int PFU_execute(ESP_PF* imp, srcObj* srcX);

int mvnpdf_execute(ESP_PF* imp, srcObj* srcX,
					std::vector<cl::Event> kernel_lst,
					cl::Event* data_event,
					cl::Event* exec_event);

int rsmpl_execute(ESP_PF* imp, srcObj* srcX);


int axis2mm_execute(ESP_PF* imp, srcObj* srcX,
					std::vector<cl::Event> kernel_lst,
					cl::Event* exec_events);
