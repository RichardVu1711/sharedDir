#ifndef READ_WRITE_CSV_H

#define READ_WRITE_CSV_H

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <utility> // std::pair
#include <stdexcept> // std::runtime_error
#include <sstream> // std::stringstream
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <fstream>

#include "../lib/Fixed_point_type.h"
#include "../lib/mat_lib.h"
#define DATA_PATH  "/mnt/sd-mmcblk0p1"

using namespace std;
typedef enum storeType{
	special,impData
} storeType;
typedef struct unit_data{
	storeType mode;
	string block;
	string var;
	int index;
	int dim[3]; // 0 = start row, 1 # of rows 2 is n column
} unit_data;


std::vector<double> read_csv(std::string filename);
void write_csv(std::string filename, std::vector<double> dataset, int row, int col);

void convert_FP(std::vector<double> input_vec, fixed_type out_FP[], int row, int col, int isMat);
std::vector<double> convert_double(fixed_type in_FP[],int row, int col, int isMat);
std::string Buid_Path(std::string folder,std::string name, int id);
std::string Save_Path(std::string Initial_path,std::string name, int id);
std::vector<double> read_csvline(std::string filename, int nrow, int ncol);
std::vector<double> read_csvMulLine(std::string filename, int row_start, int nrow, int ncol);

int readData(unit_data metaData, fixed_type* data);
int writeData(unit_data metaData, fixed_type* data);

#endif
