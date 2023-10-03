#include "Calweights.h"
//#include "lib/read_write_csv.h"

#define LOOP_FACTOR 32

// this function is used to copy a particle's vector into two vector.
// Those two generated vectors are used to calculate the estimated meas.
// This function is designed particularly for Dataflow, avoiding single-producer-consumer violations
void duplicate_data(Mat* in, int sel, Mat_S* out1, Mat_S* out2)
{
#pragma HLS inline off

	for(int i0 = 0; i0 < NUM_VAR;i0++)
	{
#pragma HLS pipeline
		out1->entries[i0*NUM_VAR] = in->entries[i0*NUM_PARTICLES+sel];
		out2->entries[i0*NUM_VAR] = in->entries[i0*NUM_PARTICLES+sel];
	}
	out1->col = 1;
	out1->row = NUM_VAR;
	out2->col = 1;
	out2->row = NUM_VAR;
}
void copy_result(	fixed_type pzx_fp[SN_NUM*2][SN_NUM*2],
					fixed_type zDiff_fp[SN_NUM*2],
					int j,
					fixed_type zDiff[NUM_PARTICLES*SN_NUM*2],
					fixed_type pzx[NUM_PARTICLES*SN_NUM*2*SN_NUM*2])
{
#pragma HLS inline off
	for(int i =0; i < SN_NUM*2;i++)
	{
		zDiff[j*N_MEAS+i] = zDiff_fp[i];
		for(int k =0; k < SN_NUM*2;k++)
		{
			pzx[j*SN_NUM*2*SN_NUM*2+ i*SN_NUM*2+ k] = pzx_fp[i][k];
		}
	}
}

extern "C"{
void CalPzxZdiff(fixed_type prtcls [NUM_VAR*NUM_PARTICLES],
				msmt* msmtinfo,
				fixed_type R_mat[N_MEAS],
				int index,
				fixed_type Pxx_[NUM_VAR*NUM_VAR],
				fixed_type zDiff[NUM_PARTICLES*N_MEAS],
				fixed_type pzx[NUM_PARTICLES*N_MEAS*N_MEAS])
{
#pragma HLS STREAM variable=prtcls
#pragma HLS STREAM variable=msmtinfo
#pragma HLS STREAM variable=R_mat
#pragma HLS STREAM variable=Pxx_
#pragma HLS STREAM variable=zDiff
#pragma HLS STREAM variable=pzx

#pragma HLS INTERFACE mode=m_axi bundle=gmem3 port=pzx offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem2 port=zDiff offset=slave

//#pragma HLS INTERFACE mode=m_axi bundle=gmem1 port=R_mat num_write_outstanding=1 max_read_burst_length=2 offset=slave
//#pragma HLS INTERFACE mode=m_axi bundle=gmem2 port=Pxx_ num_write_outstanding=1 num_write_outstanding=2 offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem1 port=msmtinfo offset=slave
#pragma HLS INTERFACE mode=m_axi bundle=gmem0 port=prtcls num_write_outstanding=1 num_write_outstanding=2 offset=slave
//
//
//#pragma HLS STABLE variable=index
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=return
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=msmtinfo
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=R_mat
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=prtcls
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=index
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=Pxx_
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=zDiff
#pragma HLS INTERFACE mode=s_axilite bundle=control_r port=pzx

//#pragma HLS inline off


	fixed_type zDiff_local[NUM_PARTICLES*N_MEAS];
	fixed_type pzx_local[NUM_PARTICLES*N_MEAS*N_MEAS];
	fixed_type pxx_local1[NUM_VAR][NUM_VAR];
	Mat prtcls_local;
	Mat_S R_local;
	msmt msmtinfo1,msmtinfo2,msmtinfo3;

	load_data(Pxx_,prtcls,R_mat,msmtinfo,pxx_local1,&prtcls_local,&R_local,&msmtinfo1,&msmtinfo2,&msmtinfo3);
	int nobs1 =  msmtinfo1.n_aoa +  msmtinfo1.n_tdoa;
	int nobs2 =  msmtinfo1.n_aoa +  msmtinfo1.n_tdoa;

//	likelihood1:for(int j =0; j < NUM_PARTICLES/LOOP_FACTOR;j++)
	likelihood:for(int j =0; j <  NUM_PARTICLES/LOOP_FACTOR;j++)
	{
		// reuse resources
		// tempX2 is Hxx, tempX3 is z_cap
#pragma HLS DATAFLOW
		Mat_S temp_X;	// particle column
		fixed_type obsJacobian[N_MEAS][NUM_VAR];
		fixed_type GISobs[N_MEAS];
		Mat_S temp_X4;	// particles column

		int idx1 = index;
		int idx2 = index;

		int j1 = j;
		int j2 = j;
		int j3 = j;
		int j4 = j;



		// duplicate data for further single-producer-consumer violation
		duplicate_data(&prtcls_local,j1,&temp_X,&temp_X4);

//		Calculate the estimated measurement based on the given particles
		ObsJacobian(&temp_X,idx1,&msmtinfo1,obsJacobian);
		GISobs_model(&temp_X4,idx2,&msmtinfo2,GISobs);


		GISPzx3(obsJacobian,pxx_local1,&R_local,pzx_local,nobs1,j3);
		zDiff_cal(GISobs,&msmtinfo3,zDiff_local,nobs2,j4);
//		write_csv("C:/ESP_PF_PLNewWeight/ESP_GSIPv3/result/ObsJacobian_COV.csv",convert_double(temp_X2.entries,1,13*6,0),6,13);
//		write_csv("C:/ESP_PF_PLNewWeight/ESP_GSIPv3/result/GISobs_model_COV.csv",convert_double(temp_X3.entries,6,1,0),1,6);

	}
	store_data(zDiff_local, pzx_local, zDiff,pzx);

}
}

void load_data(fixed_type pxx[NUM_VAR*NUM_VAR],
				fixed_type prtcls [NUM_VAR*NUM_PARTICLES],
				fixed_type R [N_MEAS],
				msmt* msmtinfo,
				fixed_type pxx_Out1[NUM_VAR][NUM_VAR],
				Mat* prtcls_temp,
				Mat_S* R_out,
				msmt* msmtinfo1,
				msmt* msmtinfo2,
				msmt* msmtinfo3)
{

#pragma HLS PIPELINE off

	pxx_cp:for(int i=0; i < NUM_VAR*NUM_VAR;i++){
//#pragma HLS UNROLL
		int idx= i/13;
		int jdx= i%13;
		pxx_Out1[idx][jdx] = pxx[i];
	}

	R_cp:for(int i=0; i < N_MEAS;i++)
	{
//#pragma HLS UNROLL
		R_out->entries[i*NUM_VAR+i] = R[i];
	}

	prtcls_cp:for(int i=0; i < NUM_VAR*NUM_PARTICLES;i++)
	{
//#pragma HLS PIPELINE
//#pragma HLS UNROLL
		prtcls_temp->entries[i]=prtcls[i];
	}
	prtcls_temp->col = NUM_PARTICLES;
	prtcls_temp->row = NUM_VAR;


	// cpy msmtinfo
	msmt:for(int i=0; i < N_MEAS;i++){
//#pragma HLS UNROLL

		msmtinfo1->validIdx[i] = msmtinfo->validIdx[i];
		msmtinfo1->z[i] = msmtinfo->z[i];
	}
	msmtinfo1->n_aoa = msmtinfo->n_aoa;
	msmtinfo1->n_tdoa = msmtinfo->n_tdoa;

	msmt2:for(int i=0; i < N_MEAS;i++){
//#pragma HLS UNROLL

		msmtinfo2->z[i] = msmtinfo1->z[i];
		msmtinfo3->z[i] = msmtinfo1->z[i];
		msmtinfo2->validIdx[i] = msmtinfo1->validIdx[i];
		msmtinfo3->validIdx[i] = msmtinfo1->validIdx[i];
	}
	msmtinfo2->n_aoa = msmtinfo1->n_aoa;
	msmtinfo2->n_tdoa = msmtinfo1->n_tdoa;
	msmtinfo3->n_aoa = msmtinfo1->n_aoa;
	msmtinfo3->n_tdoa = msmtinfo1->n_tdoa;
}
void store_data( fixed_type zDiff_local[NUM_PARTICLES*SN_NUM*2],
					fixed_type pzx_local[NUM_PARTICLES*SN_NUM*2*SN_NUM*2],
					fixed_type zDiff[NUM_PARTICLES*SN_NUM*2],
					fixed_type pzx[NUM_PARTICLES*SN_NUM*2*SN_NUM*2])
{
	size_t in_size1 = NUM_PARTICLES*N_MEAS*sizeof(fixed_type);
	size_t in_size2 = NUM_PARTICLES*N_MEAS*N_MEAS*sizeof(fixed_type);
//	memcpy(zDiff,zDiff_local,in_size1);
//	memcpy(pzx,pzx_local,in_size2);
	#pragma HLS PIPELINE off

	pzx_cp:for(int i=0; i < N_MEAS*N_MEAS*NUM_PARTICLES;i++)
	{
#pragma HLS PIPELINE
		pzx[i] =pzx_local[i];
	}

	zDiff_cp:for(int i=0; i < N_MEAS*NUM_PARTICLES;i++)
	{
#pragma HLS PIPELINE
			zDiff[i] =zDiff_local[i];
	}
}

