#include "GISPzx.h"

#include <cstdint>
#include <cstring>

void GISPzx3(fixed_type Hxx_local[N_MEAS][NUM_VAR], fixed_type pxx_local[NUM_VAR][NUM_VAR], Mat_S* R_mat,
		fixed_type pzx_local[NUM_PARTICLES*N_MEAS*N_MEAS], int n_obs, int idx)
{
#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=Hxx_local
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=pxx_local

#pragma HLS PIPELINE off

	fixed_type temp_re[N_MEAS][NUM_VAR];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=temp_re
	fixed_type t_pxx1[N_MM][NUM_VAR];
#pragma HLS ARRAY_PARTITION dim=1 type=cyclic variable=t_pxx1
	fixed_type t_re1[N_MM][1];
#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=t_re1
	fixed_type temp1[N_MM][NUM_VAR];
#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=temp1

	// Hxx*Pxx
	// idea is to take one 1 col * 1 rows.


	first_mm:for(int i=0; i < N_MEAS;i++){

		if(i < n_obs){
			// extract the row,
			for(int inst=0; inst < N_MM;inst++){
#pragma HLS LOOP_FLATTEN
				cpy_hxx:for(int j=0; j < NUM_VAR;j++){
#pragma HLS UNROLL
					temp1[inst][j] = Hxx_local[i][j];
				}
			}			// run NUM_VAR/N_MM iterations
			mm_cr:for(int k=0; k < NUM_VAR;k = k + N_MM){
//				// extract pxx columns
				cp_pxx:	for(int inst = 0; inst < N_MM * NUM_VAR; inst++) {
				    #pragma HLS UNROLL
				    int runIdx = (inst % NUM_VAR);
				    int staIdx = inst / NUM_VAR;
				    t_pxx1[staIdx][runIdx] = pxx_local[runIdx][staIdx+k];
				}
				// Conduct vector multiplication (1x13 vec * 13x1 vec = 1 scalar);
				vm_rc:for(int inst =0; inst < N_MM;inst++){
#pragma HLS UNROLL
						int i_temp = inst;
						s_macc3_13(temp1[inst],t_pxx1[inst],t_re1[inst]);
				}

				// copy this result into result vector
				for(int inst =0; inst < N_MM;inst++){
					temp_re[i][k + inst] = t_re1[inst][0];
				}
			}
			static int cnt_left =(int) (NUM_VAR/N_MM)*N_MM;
			// now conduct the left over vector multiplication (last col with the condition that N_MM =2
			// this loop is executed sequentially, without any parallelism
			leftOver_vm:for(int k = cnt_left; k < NUM_VAR; k++){
				// number of left overs = NUM_VAR module N_MM (13%2 = 1)
				// extract pxx columns
				for(int j =0; j < NUM_VAR;j++){
#pragma HLS PIPELINE
					t_pxx1[0][j] = pxx_local[k][j];

				}
				// Conduct vector multiplication (1x13 vec * 13x1 vec = 1 scalar);
				s_macc3_13(temp1[0],t_pxx1[0],t_re1[0]);

				// copy this result into result vector
				temp_re[i][k] = t_re1[0][0];
			}
		}
		else{
			for(int k =0; k < NUM_VAR;k++){
				temp_re[i][k] = 1023;
			}
		}
	}

//	cout << "Hxx * Pxx = \n";

	fixed_type temp_re2[N_MEAS][N_MEAS];
#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=temp_re2
//	fixed_type t_pxx2[N_MEAS][NUM_VAR];
//#pragma HLS ARRAY_PARTITION dim=2 type=complete variable=t_pxx2

	// calculate Re*Hxx'
	// (Hxx*Pxx)*Hxx'
	se_mm:for(int i=-0; i < N_MEAS;i++){
		if(i < n_obs){
			// extract the row,
			smm_cr:for(int k=0; k < N_MEAS;k=k+1){
#pragma HLS PIPELINE
				if(k < n_obs){
					// extract the row of Hxx or column of Hxx',
					// a row of Hxx is a column of Hxx'(transposed)
//					pzx_cal_label2:for(int j=0; j< NUM_VAR;j++){
//#pragma HLS UNROLL
//						t_pxx2[k][j] = Hxx_local[k][j];
//					}
					// parallelise the multiplication
					fixed_type temp = 0;
#pragma HLS BIND_OP variable=temp op=mul impl=dsp
					// multiply
					mul2:for(int j=0; j< NUM_VAR;j++){
#pragma HLS UNROLL factor=4
						temp = temp + temp_re[i][j] * Hxx_local[k][j];
					}
					temp_re2[i][k] = temp;
				}
				else{
					temp_re2[i][k] = 1023;
				}
			}
		}
		else{
			for(int k =0; k < N_MEAS;k++){
				temp_re2[i][k] = 1023;
			}
		}
	}

	// add std
	for(int i=0; i < N_MEAS;i++){
		temp_re2[i][i] = temp_re2[i][i] + R_mat->entries[i*NUM_VAR+i];
	}

	for(int i=0; i < N_MEAS;i++){
		for(int j=0; j < N_MEAS;j++){
#pragma HLS PIPELINE
			pzx_local[idx*N_MEAS*N_MEAS+ i*N_MEAS + j] = (temp_re2[i][j] + temp_re2[j][i])/2;
		}
	}
}

// vector multiplication between  two vector of 13 elements.
void s_macc3_13(fixed_type temp1[NUM_VAR],
				fixed_type temp2[NUM_VAR],
				fixed_type out[1]){


	fixed_type temp = 0;
#pragma HLS BIND_OP variable=temp op=mul impl=dsp
	// multiply
	mul:for(int j=0; j< NUM_VAR;j++){
#pragma HLS PIPELINE
		temp = temp + temp1[j] * temp2[j];
	}
//	fixed_type temp3 = temp;
	// write output
	out[0] = temp;
}

void zDiff_cal(fixed_type z_cap[N_MEAS],
			msmt* msmtinfo,
			fixed_type zDiff_local[NUM_PARTICLES*N_MEAS],
			int n_obs, int idx)
{
	zDiff_loop:for(int i=0; i < N_MEAS;i++)
	{
		if(i < n_obs){
			zDiff_local[idx*N_MEAS+ i] = -z_cap[i] + msmtinfo->z[i];
		}
		else{
			zDiff_local[idx*N_MEAS+ i] = 1023;
		}

	}
}


