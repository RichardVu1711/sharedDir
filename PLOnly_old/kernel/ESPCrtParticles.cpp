#include "ESPCrtParticles.h"

void ESPCrtParticles(Mat_S* state_input, Mat_S* Pxx_input, Mat* particles,
		fixed_type rnd_sigma[NUM_PARTICLES*NUM_VAR],fixed_type rnd_rk4[4*NUM_VAR])
{
    Mat_S Pxx;
    init_mat(&Pxx,Pxx_input->row,Pxx_input->col);
    copyvalue_S(Pxx_input,&Pxx);
//    cout<<"Pxx before update";
//    showmat_S(&Pxx);


    Mat_S X_mean;
    init_mat(&X_mean,state_input->row,state_input->col);
    copyvalue_S(state_input,&X_mean);     // fixed 13x1
//    showmat_S(&X_mean);
    Mat_S X_pro;
    init_mat(&X_pro,2,NUM_VAR);   // fixed 2x13
    fixed_type t_step = 1;
    fixed_type t_span[2] = {0,1};
    //generate Ns particles
    Mat sigMat;
    sigMat.col = NUM_PARTICLES;
    sigMat.row = NUM_VAR;
	int k=0;
//	showmat_S(&Pxx);
//	cout << "HelloWOrld";
	for (int i = 0; i < NUM_VAR; i++)
	{
		ap_fixed<WORD_LENGTH,INT_LEN> temp = get_ele_S(&Pxx,i,i);
		fixed_type sqrtRe = hls::sqrt(temp);
		for(int j=0; j < NUM_PARTICLES;j++)
		{
			// Matlab formula normrnd = random*sigma + mu
			// random is taken from rnd_data, sigma  =  sqrt(pxx(i,i)), and mu =0
			fixed_type r = rnd_sigma[k++] * sqrtRe + 0;
			set_ele(&sigMat,i,j,r);
		}
	}
//    cout<< "sigMat:";
//    show_mat(&sigMat);
//     mean propagation
	Mat_S X_meanpro;
	init_mat(&X_meanpro,NUM_VAR,1);
    rk4(&X_mean,&X_meanpro,rnd_rk4);
//    showmat_S(&X_meanpro);
//    Mat_S getRow;
//    init_mat(&getRow,1,X_pro.col);
//    getRowMat_S(&X_pro,X_pro.row-1,&getRow);


    // Using the second row for futher computation

    // Jacobian is a constant regardless of time and mean state
    // Therefore, expm(Jacobian) will also be a constant number !!
    Mat_S J;
    init_mat(&J,NUM_VAR,NUM_VAR);
    for (int i=0; i < J.col;i++)
    {
        for(int j =0; j < J.row;j++)
        {
            set_ele_S(&J,i,j,jacobian[i*  J.row + j]);
        }
    }


    // data_checking to ensure that all inputs are correct before the computation
    // J->entries: exponential array

    Mat_S sigma_col;
    init_mat(&sigma_col,sigMat.row,1);
    Mat_S final;
    init_mat(&final,NUM_VAR,1);
    Mat_S temp_cal;
    init_mat(&temp_cal,J.row,sigma_col.col);
    for(int i=0;i<NUM_PARTICLES;i++)
    {
        // based on the literature  review, the calculation of particles could be compressed or simplified as
        // prtcls = X_meanpro + e^J * sigma


        getColMat_L2S(&sigMat,i,&sigma_col);
        multiply_S(&J,&sigma_col,&temp_cal);
        sum_S(&X_meanpro,&temp_cal,&final);
        setColMat_S2L(particles,i,&final);
    }
}
