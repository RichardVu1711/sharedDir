#pragma once
#include "ESPObj.h"
#include "srcObj.h"

#include "xcl2.hpp"
#include "experimental/xrt_profile.h"

#include "../random_generator/RNG_withSeed.h"
#include "../rk4/rk4.h"
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



int smplPhase_execute(ESP_PF* imp, srcObj* srcX);

int rk4_execute(ESP_PF* imp, srcObj* srcX);
int ESPCrtParticles_execute(ESP_PF* imp, srcObj* srcX,
							std::vector<cl::Event> kernel_events,
							cl::Event* data_events,
							cl::Event* exec_events);
int sigma_execute(ESP_PF* imp, srcObj* srcX,
				std::vector<cl::Event> kernel_events,
				cl::Event* data_events,
				cl::Event* exec_events);
int mPxx_execute(ESP_PF* imp, srcObj* srcX,
				std::vector<cl::Event> kernel_events,
				cl::Event* data_events,
				cl::Event* exec_events);

int calwPhase_execute(ESP_PF* imp, srcObj* srcX, int index);
int calW_execute(ESP_PF* imp, srcObj* srcX, int index);

