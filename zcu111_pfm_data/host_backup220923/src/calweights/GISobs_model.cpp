#include "GISobs_model.h"

void GISobs_model(Mat_S* prtcl_X, int step, msmt* msmtinfo, Mat_S* z_cap)
{
	fixed_type SNx[6] = {0,0,-1.9561,-739.8094,-383.0758,-2.3068};
	// SNx is the location of the sensor node in x-y coordinator
	// SN1_x SN1_y SN2_x SN2_y SN3_x SN3_y


	z_cap->row = SN_NUM*2;
	z_cap->col = 1;

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

		double a = diffY[i];
		double b = diffX[i];
		double temp1 = atan2(b,a);
		fixed_type temp= temp1;
		temp += get_ele_S(prtcl_X,10+i,0);
		set_ele_S(z_cap,i,0,temp);
	}
	// calculate TDOA est

	// 1023 means invalid
	// 3 is maximum values for n_row or n_col
	// aoaIdx contains what node is triggered SN1 =01, SN2= 02, SN3 =03
	// tdoaIdx contains what node is triggered 12 = 01, 13= 02, 23 = 03
//	std::cout << "\n" << "n_aoa " << n_aoa << "\n";
	for(int i =0; i < SN_NUM;i++)
	{
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
		double a= (diffX[i1]/devx[i1])*(diffX[i1]/devx[i1]) + (diffY[i1]/devx[i1])*(diffY[i1]/devx[i1]);

		double b= (diffX[i2]/devx[i2])*(diffX[i2]/devx[i2]) + (diffY[i2]/devx[i2])*(diffY[i2]/devx[i2]);

		double temp1 = sqrt(a);
		double temp2 = sqrt(b);
		temp_sqrt1 = temp1;
		temp_sqrt2 = temp2;
		fixed_type fs_c =FS_C;
		double temp_re = fs_c*(temp_sqrt1*devx[i1] - temp_sqrt2*devx[i2])
					+get_ele_S(prtcl_X,4+i,0) +get_ele_S(prtcl_X,7+i,0)*(step+2);
		set_ele_S(z_cap,SN_NUM+i,0,temp_re);
	}
	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;
	fixed_type temp_array[SN_NUM*2];
	int k =0;
	// checkin AOA measurement
	for (int i =0; i < SN_NUM;i++)
		{
			if(aoaTag[i] != 1023)
			{
				temp_array[k] =aoaTag[i]-1;
				k++;
			}
		}

	// currently k=n_aoa. keep that one for the incoming tdoa check
	for (int i =0; i < SN_NUM;i++)
	{
		if(tdoaTag[i] != 1023)
		{
			temp_array[k] =n_aoa + tdoaTag[i]-1;
			k++;
		}
	}
	// at this point  k = n_aoa + n_tdoa
	// if not => there must be something wrong
	// this "if" is just a debug point
	if(k != n_aoa + n_tdoa)
	{
		printf("GISobs_model is wrong !!!, k = %d, n_aoa+n_tdoa-1= %d\n",k,n_aoa + n_tdoa-1);
	}
	// now update the new valid measurement
	for(k= 0; k > n_aoa + n_tdoa; k++)
	{
		z_cap->entries[k*NUM_VAR] =z_cap->entries[temp_array[k]*NUM_VAR] ;
	}
	z_cap->row = n_aoa + n_tdoa;
	z_cap->col = 1;
//	showmat_S(&z_cap);
//	return z_cap;
}
