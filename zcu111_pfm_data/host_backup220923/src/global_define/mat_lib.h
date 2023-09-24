#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <iostream>
#include <typeinfo>

#include "global_define.h"
#include "mat_type.h"


using namespace std;
void init_mat(Mat_S* M, int col, int row);
fixed_type get_ele_S(Mat_S* M,int r,int c);
int set_ele_S(Mat_S* M,int r,int c,fixed_type d);
void copyvalue_S(Mat_S* A, Mat_S* B);
void CTranspose_S(Mat_S* A, Mat_S* new_A);
void getColMat_S (Mat_S* A, int selected_col, Mat_S* SavedMat);
void multiply_S(Mat_S* A,Mat_S* B, Mat_S*C_Result);
void scalermultiply_S(Mat_S* M,fixed_type c, Mat_S* B_Re);
void sum_S(Mat_S* A,Mat_S* B, Mat_S* C_Re);
void minus_S(Mat_S* A,Mat_S* B, Mat_S* C_Re);
void setRowMat_S(Mat_S* A, int rowIdx, fixed_type* insertedRow);
void getRowMat_S (Mat_S* A, int selected_row, Mat_S* SavedMat);
void showmat_S(Mat_S* A);

// Bigger Matrix
void setColMat_S2L (Mat* A, int selected_col, Mat_S* addedCol);
void newmat(Mat* A, int n_row, int n_col);
void getColMat_L2S (Mat* A, int selected_col, Mat_S* SavedMat);
void set_ele(Mat* A, int row, int col, fixed_type in_val);
void show_mat(Mat* X);

//  hasmap handy function
// type 1, MAT_S => row*NUM_VAR+col
// type 2, symmetric Type
// type 3, Pzx => row*N_MEAS+col
int hash_map(int row, int col, int type);

