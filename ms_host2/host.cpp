/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include "xcl2.hpp"
#include "obj_code/srcObj.h"
#include "obj_code/ESPObj.h"

#include <algorithm>
#include <vector>
#define DATA_SIZE 4096

int main(int argc, char** argv) {

	std::string datapth = "";
	srcObj srcx [N_SRC];
	ESP_PF imp(&argc,&argv);
    for(int i=0; i < N_SRC;i++){
        srcx[i] = srcObj("/mnt/test_data/obsVal2/Init/obsVal2.csv",imp.esp_control.context,imp.esp_control.q[i]);
    }

    bool match = true;
    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
