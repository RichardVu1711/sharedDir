#include "read_write_csv.h"
#include "mat_lib.h"
using namespace std;
std::vector<double> read_csv(std::string filename){
    // Reads a CSV file into a vector of std::vector<double> 

    // Create a vector of std::vector<double> to store the result
	std::vector<double> result;

    // Create an input filestream
    std::ifstream myFile(filename);

//    // Make sure the file is open
//    if(!myFile.is_open()) return null;
    // Helper vars
    std::string line;
    double val;
    // Read data, line by line
    while(std::getline(myFile, line))
    {
        // Create a stringstream of the current line
        std::stringstream ss(line);
//      std::cout << "STR: "<< line << "\t";
        // Keep track of the current column index
        int colIdx = 0;
        
        // Extract each integer
        while(ss >> val)
        {
            // Add the current integer to the 'colIdx' column's values vector
            result.push_back(val);
//          std::cout << std::fixed << std::setprecision(6) << val;
//			std::cout << " Read \n";
            // If the next token is a comma, ignore it and move on
            if(ss.peek() == ',') ss.ignore();
            
            // Increment the column index
            colIdx++;
        }

    }

    // Close file
    myFile.close();

    return result;
}

std::vector<double> read_csvline(std::string filename, int nrow, int ncol)
{
    // Reads a CSV file into a vector of std::vector<double>

    // Create a vector of std::vector<double> to store the result
	std::vector<double> result;

    // Create an input filestream
    std::ifstream myFile(filename);

//    // Make sure the file is open
//    if(!myFile.is_open()) return null;
    // Helper vars
    std::string line;
    double val;
    int rowIdx = 0;
    // Read data, line by line
    while(std::getline(myFile, line))
    {
        // Create a stringstream of the current line
        std::stringstream ss(line);
//      std::cout << "STR: "<< line << "\t";
        // Keep track of the current column index
        int colIdx = 0;

        // Extract each integer
        while(ss >> val)
        {
            // Add the current integer to the 'colIdx' column's values vector
            if(rowIdx == nrow) result.push_back(val);
//          std::cout << std::fixed << std::setprecision(6) << val;
//			std::cout << " Read \n";
            // If the next token is a comma, ignore it and move on
            if(ss.peek() == ',') ss.ignore();

            // Increment the column index
            colIdx++;
        }
//        if(rowIdx == nrow) break;
        rowIdx++;

    }

    // Close file
    myFile.close();

    return result;
}

void write_csv(std::string filename, std::vector<double> dataset, int row, int col){
    // Make a CSV file with one or more columns of integer values
    // Each column of data is represented by the double data <column data>
    //   as std::vector<double> 
    // The dataset is represented as a vector of these columns
    // Note that all columns should be the same size
    
    // Create an output filestream object
    std::ofstream myFile(filename,std::ios::app);
    int k =0;
    // Send data to the stream
    for(int i = 0; i < row; ++i)
    {
        for(int j = 0; j < col; ++j)
        {
            myFile << dataset.at(k);
//            
//            std::cout << std::fixed << std::setprecision(6) << dataset.at(k);
//            std::cout << " Write \n";
            if(j != dataset.size() - 1) myFile << ","; // No comma at end of line
            k++;
        }
        myFile << "\n";
    }
    
    // Close the file
    myFile.close();
}

void convert_FP(std::vector<double> input_vec, fixed_type out_FP[], int row, int col)
{
//	printf( "\n\n");
	int size = input_vec.size();
//	printf("size is: %d\n ",size );

	int n_col = NUM_PARTICLES;
	if (size < NUM_VAR*NUM_VAR+1)
		n_col = NUM_VAR;
	int k = 0;
	for(int i =0; i < row;i++)
	{
		for(int j =0; j < col;j++)
		{
			double temp = input_vec.at(k++);
			out_FP[i*n_col+j] = temp;
		}
//		printf("\n");
	}
}


std::vector<double> convert_double(fixed_type in_FP[],int row, int col)
{
		int size = sizeof(in_FP)/sizeof(in_FP[0]);
		int n_row = NUM_VAR;
		int n_col = NUM_PARTICLES;
		if (size < NUM_VAR*NUM_VAR+1)
			n_col = NUM_VAR;
		int k = 0;
		std::vector<double> out_du (row*col);
		for(int i =0; i < row;i++)
		{
			for(int j =0; j < col;j++)
			{
				out_du.at(k++) =  in_FP[i*n_col+j];
			}
		}
		return out_du;
}

std::string Buid_Path(std::string folder,std::string name, int id)
{
	std::string path = DATA_PATH + std::string("/") + folder + std::string("/") + name + std::string("/")
	+ name + std::string("_part") + std::to_string(id) + std::string(".csv");
	return path;
}

std::string Save_Path(std::string Initial_path,std::string name, int id)
{
	std::string path = name +  std::string("_PL_") + std::to_string(id) + std::string(".txt");
	return path;
}





