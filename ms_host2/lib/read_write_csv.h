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
#define DATA_PATH  "C:/PF_folder/Data"

using namespace std;

std::vector<double> read_csv(std::string filename);
void write_csv(std::string filename, std::vector<double> dataset, int row, int col);

void convert_FP(std::vector<double> input_vec, fixed_type out_FP[], int row, int col, int isMat);
std::vector<double> convert_double(fixed_type in_FP[],int row, int col, int isMat);
std::string Buid_Path(std::string folder,std::string name, int id);
std::string Save_Path(std::string Initial_path,std::string name, int id);
std::vector<double> read_csvline(std::string filename, int nrow, int ncol);
std::vector<double> read_csvMulLine(std::string filename, int row_start, int nrow, int ncol);


#endif
