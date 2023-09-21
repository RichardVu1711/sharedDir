#include "../calW/GISobs_model.h"

void GISobs_model(Mat_S* prtcl_X, int step, msmt* msmtinfo, Mat_S* z_cap)
{
	fixed_type SNx[6] = {0,0,-1.9561,-739.8094,-383.0758,-2.3068};
	// SNx is the location of the sensor node in x-y coordinator
	// SN1_x SN1_y SN2_x SN2_y SN3_x SN3_y

	for(int i=0; i < N_MEAS;i++)
	{
		z_cap->entries[i*NUM_VAR] = 1023;
	}
	z_cap->row = N_MEAS;
	z_cap->col = 1;

	Mat_S z_temp;
	z_temp.row = N_MEAS;
	z_temp.col = 1;


	fixed_type x = get_ele_S(prtcl_X,0,0);
	fixed_type y = get_ele_S(prtcl_X,1,0);

	fixed_type diffX[SN_NUM]={0,0,0};
	fixed_type diffY[SN_NUM]={0,0,0};

	fixed_type devx [SN_NUM] ={1,1,1};
	int aoaTag[3];
	int tdoaTag[3];
	for(int i = 0; i < SN_NUM;i++)
	{
		aoaTag[i]= msmtinfo->aoaIdx[i];
		tdoaTag[i] = msmtinfo->tdoaIdx[i];
		diffX[i] = x - SNx[i*2];
		diffY[i] = y - SNx[i*2+1];

		// check if the X and Y distance is too big
		// if they are greater than 30, needs to scale them down before calculating the square root
		// or square of these values
		if(diffX[i] > 30 || diffY[i] > 30 || diffX[i] < -30 || diffY[i] < -30) devx[i] =128;
	}
	// In this method: we calculate all of 6 SNs first, then we re-organised the particles order
	// based on
	// calculate AOA est
	for(int i =0; i < SN_NUM;i++)
	{
		if(msmtinfo->validIdx[i]){
			ap_fixed<WORD_LENGTH,INT_LEN> a = diffY[i];
			ap_fixed<WORD_LENGTH,INT_LEN> b = diffX[i];
	//		cout << a <<", " << b << "\n";
			fixed_type temp = hls::atan2(b,a);
			temp += get_ele_S(prtcl_X,10+i,0);
			set_ele_S(&z_temp,i,0,temp);
		}
	}
	// calculate TDOA est

	// 1023 means invalid
	// 3 is maximum values for n_row or n_col
	// aoaIdx contains what node is triggered SN1 =01, SN2= 02, SN3 =03
	// tdoaIdx contains what node is triggered 12 = 01, 13= 02, 23 = 03
	for(int i =0; i < N_TDOA;i++)
	{
		// only compute if valid
		if(msmtinfo->validIdx[i+N_AOA]){
			fixed_type temp_sqrt1,temp_sqrt2;
			int i1,i2;
			switch(i+1)
			{
				case 1: // 1 is 12
					i1 = 0;
					i2 = 1;
					break;
				case 2: // 2 is 13
					i1 = 0;
					i2 = 2;
					break;
				case 3: // 3 is 23
					i1 = 1;
					i2 = 2;
					break;
			}
			ap_fixed<WORD_LENGTH,INT_LEN> a= (diffX[i1]/devx[i1])*(diffX[i1]/devx[i1]) + (diffY[i1]/devx[i1])*(diffY[i1]/devx[i1]);

			ap_fixed<WORD_LENGTH,INT_LEN> b= (diffX[i2]/devx[i2])*(diffX[i2]/devx[i2]) + (diffY[i2]/devx[i2])*(diffY[i2]/devx[i2]);
			temp_sqrt1 = hls::sqrt(a);
			temp_sqrt2 = hls::sqrt(b);

			fixed_type fs_c =FS_C;
			fixed_type temp_re = fs_c*(temp_sqrt1*devx[i1] - temp_sqrt2*devx[i2])
						+get_ele_S(prtcl_X,4+i,0) +get_ele_S(prtcl_X,7+i,0)*(step+2);
			// continue after TDOA calculation
			set_ele_S(&z_temp,N_AOA+i,0,temp_re);
		}
	}

	// check valid TDOA and AOA measurement.
	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;

	int outputIdx =0;
	for(int i=0; i < N_MEAS;i++){
		if(msmtinfo->validIdx[i])
		{
			z_cap->entries[outputIdx*NUM_VAR] = z_temp.entries[i*NUM_VAR];
			outputIdx++;

		}
	}
	if(outputIdx !=  (n_aoa + n_tdoa)){
		printf("The number of Valid data is not match !!\n");
		//make it failed fast
		for(int i=0;  i< N_MEAS;i++)
		{
			z_cap->entries[i*NUM_VAR] = 1023;
		}

	}
	z_cap->row = n_aoa + n_tdoa;
	z_cap->col = 1;
//	showmat_S(z_cap);

}
