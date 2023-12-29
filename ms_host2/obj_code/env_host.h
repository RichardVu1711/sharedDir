#pragma once
#include "ESPObj.h"
#include "srcObj.h"


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



