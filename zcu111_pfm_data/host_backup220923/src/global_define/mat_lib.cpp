#include"mat_lib.h"

void showmat_S(Mat_S* A){

#ifndef __SYNTHESIS__
	if(A->row>0&&A->col>0){
		printf("\nMatrix:\n[");
		for(int i=0;i<A->row;i++){
			for (int j=0;j<A->col;j++){
				double temp = A->entries[i*NUM_VAR +j];
				if(j<A->col-1){
					printf("%f\t",temp);
				}else{
					printf("%f",temp);
				}
			}
			if(i<A->row-1){
				printf("\n");
			}else{
				printf("]\n");
			}
		}
		printf("\n");
	}else{
		printf("[]");
	}
#endif
}

void setRowMat_S(Mat_S* A, int rowIdx, fixed_type* insertedRow)
{

	for(int i =0; i < A->col; i++)
	{
		A->entries[rowIdx*NUM_VAR +i] = insertedRow[i];
	}
}

void init_mat(Mat_S* M, int row, int col)
{

	// Matrix is initialized to 1023 values
	// row and col is assigned to 1;
	for(int i = row; i <NUM_VAR;i++)
	{
		for(int j = col; j<NUM_VAR;j++)
		{
			M->entries[i*NUM_VAR+j] = 1023;
		}
	}
	M->col = col;
	M->row = row;
}


fixed_type get_ele_S(Mat_S* M,int r,int c){

    if(r >= M->row || c >= M->col)
	{
    	return 1023;
	}
	fixed_type d=M->entries[(r)*NUM_VAR+c];
	return d;
}

// return 1 if go through.
// otherwise return 1023;
int set_ele_S(Mat_S* M,int r,int c,fixed_type d){

	if(r >= M->row || c >= M->col)
	{
		return 1023;
	}
	else M->entries[(r)*NUM_VAR+c]=d;
	return 1;
}
void copyvalue_S(Mat_S* A, Mat_S* B)
{

    int k=0;
	for(int i=0;i<A->row;i++){
		for(int j=0;j<A->col;j++)
		{
			B->entries[i*NUM_VAR +j]=A->entries[i*NUM_VAR +j];
		}
	}
}
void CTranspose_S(Mat_S* A, Mat_S* new_A)
{

	int row =  A->row;
	int col = A->col;
	int i,j;
	for(i=0;i <row;i++)
	{
		for(j=0;j<col;j++)
		{
			fixed_type temp = get_ele_S(A,i,j);
			set_ele_S(new_A,j,i,temp);
		}
	}
}

void getColMat_S (Mat_S* A, int selected_col, Mat_S* SavedMat)
{

	int  i;

	for(i =0; i < A->row;i++)
	{
		set_ele_S(SavedMat,i,0,get_ele_S(A,i,selected_col));
	}
	// return sel_col;
}

void multiply_S(Mat_S* A,Mat_S* B, Mat_S*C_Result){

	int r1=A->row;
	int r2=B->row;
	int c1=A->col;
	int c2=B->col;
	if (r1==1&&c1==1){
		scalermultiply_S(B,A->entries[0],C_Result);
		return;
	}else if (r2==1&&c2==1){
		scalermultiply_S(A,B->entries[0],C_Result);
		return;
	}

	for(int i=0;i<r1;i++){
		for(int j=0;j<c2;j++){
			fixed_type de=0;
			for(int k=0;k<r2;k++){
				de+=A->entries[(i)*NUM_VAR+k]*B->entries[(k)*NUM_VAR+j];
			}
			C_Result->entries[(i)*NUM_VAR+j]=de;
		}
	}
}

void scalermultiply_S(Mat_S* M,fixed_type c, Mat_S* B_Re){

	for(int i=0;i<M->row;i++){
		for(int j=0;j<M->col;j++){
			B_Re->entries[(i)*NUM_VAR+j]=M->entries[(i)*NUM_VAR+j]*c;
		}
	}
}

void sum_S(Mat_S* A,Mat_S* B, Mat_S* C_Re){

	int r=A->row;
	int c=A->col;
	for(int i=0;i<r;i++){
		for(int j=0;j<c;j++){
			C_Re->entries[i*NUM_VAR+j]=A->entries[i*NUM_VAR+j]+B->entries[i*NUM_VAR+j];
			double temp = C_Re->entries[i*NUM_VAR+j];
			double temp2 = B->entries[i*NUM_VAR+j];
			double temp3 = temp + temp2;
		}
	}
}


void minus_S(Mat_S* A,Mat_S* B, Mat_S* C_Re)
{

	// C_re = A-B
	int r=A->row;
	int c=A->col;
	for(int i=0;i<r;i++)
	{
		for(int j=0;j<c;j++)
		{
			C_Re->entries[i*NUM_VAR+j]=A->entries[i*NUM_VAR+j]-B->entries[i*NUM_VAR+j];
		}
	}
}
void getRowMat_S (Mat_S* A, int selected_row, Mat_S* SavedMat)
{

	int  i;
	for(i =0; i < A->col;i++)
	{
		set_ele_S(SavedMat,0,i,get_ele_S(A,selected_row,i));
	}
}


// =========================== Bigger Matrix Type NUM_VAR x NUM_PARTICLES ====================================
void setColMat_S2L (Mat* A, int selected_col, Mat_S* addedCol)
{

	int  i;
	for(i =0; i < A->row;i++)
	{
		// Mat index: r*NUM_PARTICLES + c	NUM_VAR x NUM_PARTICLES
		// Mat_S index: r*NUM_VAR + c		NUM_VAR x NUM_VAR
		A->entries[i*NUM_PARTICLES+selected_col] = addedCol->entries[i*NUM_VAR];
	}
}

void newmat(Mat* M, int r,int c){

	M->row = r;
    M->col = c;			
	int i;
    for(i=0;i<NUM_VAR*NUM_PARTICLES;i++)
	{
			M->entries[i] = 1023;
	}
}

void getColMat_L2S (Mat* A, int selected_col, Mat_S* SavedMat)
{

	for(int i =0; i < A->row;i++)
	{
		// Mat index: r*NUM_PARTICLES + c	NUM_VAR x NUM_PARTICLES
		// Mat_S index: r*NUM_VAR + c		NUM_VAR x NUM_VAR
		SavedMat->entries[i*NUM_VAR+0] = A->entries[i*NUM_PARTICLES+selected_col];
	}
	// return sel_col;
}


void set_ele(Mat* A, int row, int col, fixed_type in_val)
{

	A->entries[row*NUM_PARTICLES + col] =in_val;
}

void show_mat(Mat* X)
{
#ifndef __SYNTHESIS_
	cout<<"\nMatrix: \n";

	int i =0;
	int j =0;
	for(i = 0; i < X->row;i++)
	{
		for(j=0;j< X->col;j++)
		{
			double temp = X->entries[i*NUM_PARTICLES+j];
			printf("%f\t",temp);
		}
		printf("\n");
	}
	printf("End====\n");
#endif
}


//hash-map handy function
int hash_map(int row, int col, int type)
{
	// type = 1 => Mat_S
	int result =0;
	switch(type)
	{
		case 1:
			result = row*NUM_VAR+col;
			break;
		case 2: {
			int temp = row;
			if (row > col)
			{
				temp = col;
			}
			result =  row*NUM_VAR+col - temp*(temp+1)/2;
			break;
		}
		case 3:
			result = row*N_MEAS + col;
			break;
		default:
		{
			printf("ERROR: Invalid hashmap type!!\n\n");
			result = -1;
		}
			break;
	}
	return result;
}
