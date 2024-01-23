#include "ESPCrtParticles.h"
void store_data_crt(fixed_type X_meanpro[NUM_VAR],
					fp_str& sigMat,	// 13*1024 internal stream
					Mat_S* X_meanpro_out,
					Mat* sigMat_out)
{
	for(int i=0; i < NUM_VAR*NUM_PARTICLES;i++)
	{
#pragma HLS PIPELINE
		sigMat_out->entries[i] = sigMat.read();
	}

	for(int i=0; i < NUM_VAR;i++)
	{
#pragma HLS UNROLL
		X_meanpro_out->entries[i*NUM_VAR] = X_meanpro[i];
	}

	sigMat_out->col = NUM_PARTICLES;
	sigMat_out->row = NUM_VAR;
	X_meanpro_out->col = 1;
	X_meanpro_out->row = NUM_VAR;

}

void load_data_crt( Mat* prtcls_out,
					fp_str& prtcls)
{
	for(int i=0; i < NUM_VAR*NUM_PARTICLES;i++)
	{
#pragma HLS PIPELINE
		prtcls.write(prtcls_out->entries[i]);
//		prtcls2.write(prtcls_out->entries[i]);

	}
}
extern "C"
{
void ESPCrtParticles(fixed_type X_meanpro[NUM_VAR],
					fp_str& sigMat,	// 13*1024
					fp_str& prtcls)
{
	#pragma HLS PIPELINE off

	Mat_S X_meanpro_local;
	Mat prtcls_local;
	prtcls_local.col = NUM_PARTICLES;
	prtcls_local.row = NUM_VAR;
	Mat sigMat_local;
	store_data_crt(X_meanpro,sigMat,&X_meanpro_local,&sigMat_local);
    // Jacobian is a constant regardless of time and mean state
    // Therefore, expm(Jacobian) will also be a constant number !!

	//UPDATE 16/10
	// TODO: Implement Exponential Matrix
	// This may be dumbed into PS
    Mat_S J;
    init_mat(&J,NUM_VAR,NUM_VAR);
    for (int i=0; i < NUM_VAR;i++)
    {
        for(int j =0; j <NUM_VAR;j++)
        {
#pragma HLS UNROLL
            set_ele_S(&J,i,j,jacobian[i*  J.row + j]);
        }
    }

    // data_checking to ensure that all inputs are correct before the computation
    // J->entries: exponential array
    Mat_S sigma_col;
    init_mat(&sigma_col,NUM_VAR,1);
    Mat_S final;
    init_mat(&final,NUM_VAR,1);
    Mat_S temp_cal;
    init_mat(&temp_cal,NUM_VAR,NUM_VAR);
    for(int i=0;i<NUM_PARTICLES;i++)
    {
        // based on the literature  review, the calculation of particles
    	// could be simplified as
        // prtcls = X_meanpro + e^J * sigma
        getColMat_L2S(&sigMat_local,i,&sigma_col);
        multiply_S(&J,&sigma_col,&temp_cal);
        sum_S(&X_meanpro_local,&temp_cal,&final);
        setColMat_S2L(&prtcls_local,i,&final);
    }
    load_data_crt(&prtcls_local,prtcls);

}
}
