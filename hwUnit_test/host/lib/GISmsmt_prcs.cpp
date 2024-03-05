#include "GISmsmt_prcs.h"


void outlier_detector(fixed_type obsVals[10],fixed_type cAvg[N_MEAS], int index, fixed_type nAvg[N_MEAS], fixed_type fil_data[N_MEAS]){

	fixed_type idx = (double) index;
	fixed_type aoastd = (double)AOASTD;
	fixed_type tdoastd = (double)TDOASTD;

	// Calculate the z-scores = (X - nAvg)/std_meas
	for(int i=0; i < N_MEAS;i++){
		fixed_type X = obsVals[i];
		//calculate the new moving avg based on the incoming data
		nAvg[i] = (cAvg[i]*(idx-1)/idx) + X/idx;
		//calculate the (X - nAvg)
		fixed_type diff = X - nAvg[i];
		fixed_type  z_scores =0;
		//calculate z-scores:
		if(i < N_AOA){// use aoastd
			z_scores = diff/aoastd;
		}
		else{	//use tdoastd
			z_scores = diff/tdoastd;
		}

		// check if the threshold is exceeded.
		if(z_scores > OL_THOLD)
			fil_data[i] = 1023;
		else
			fil_data[i] =  X;
	}
}


// msmt information
// 1023 means invalid
// 3 is maximum values for n_row or n_col
// aoaIdx contains what node is triggered SN1 =01, SN2= 02, SN3 =03
// tdoaIdx contains what node is triggered 12 = 01, 13= 02, 23 = 03
msmt msmt_prcs(fixed_type* obsVals, int index, fixed_type cAvg[N_MEAS], fixed_type nAvg[N_MEAS])
{
	// obsVals dimension is 1x10
	//msmt.z dimension is 6x1
	msmt msmtinfo;
	fixed_type fil_data[N_MEAS];
	msmtinfo.n_aoa= 0;
	msmtinfo.n_tdoa= 0;
	// initialise valid flag
	for(int i=0; i < N_MEAS;i++)
	{
		msmtinfo.validIdx[i] = 0;
	}
	int k = 0;
	outlier_detector(obsVals, cAvg, index, nAvg, fil_data);


	AOA_cal:for(int i =0; i < SN_NUM;i++)
	{
		fixed_type AOA = fil_data[i];
//		fixed_type AOA = obsVals->entries[i];
//		fixed_type TDOA = obsVals.entries[i+3];
		if(AOA !=1023){
//			if(AOA> 180) AOA = -(360 - AOA);
			AOA = deg2Rad(AOA);
			// set AOA values to entries
			msmtinfo.z[k++] = AOA;
			msmtinfo.n_aoa++;
			msmtinfo.aoaIdx[i] = i+1;
			// flag this data is valid
			msmtinfo.validIdx[i] = 1;
		}
		else{
			// receive invalid values
			// flag this data is invalid
			msmtinfo.validIdx[i] = 1;
			msmtinfo.aoaIdx[i] = 1023;
		}
	}
//		fixed_type TDOA =get_ele_S(obsVals,0,i+3);
	for(int i =0; i < SN_NUM;i++)
	{
		fixed_type TDOA = fil_data[i+3];
		if(TDOA !=1023)
		{
			// set TDOA values to entries
			msmtinfo.z[k++] = TDOA;
			msmtinfo.n_tdoa++;
			msmtinfo.tdoaIdx[i] = i+1;
			// continuous for checking tdoa
			msmtinfo.validIdx[i+N_AOA] = 1;
		}
		else
		{
			msmtinfo.validIdx[i+N_AOA] = 0;
			msmtinfo.tdoaIdx[i] = 1023;
		}
	}
//	cout << "\nValid Data Idx: \n";
//	for(int i=0; i < N_MEAS;i++){
//		cout << msmtinfo.validIdx[i] << ", ";
//	}
//	cout << "\nValid Data: \n";
//	for(int i=0; i < N_MEAS;i++){
//		cout << msmtinfo.z[i] << ", ";
//	}
//	cout << "\n";

	return msmtinfo;
}

fixed_type deg2Rad(fixed_type deg)
{
	fixed_type pi = M_PI;
	return deg*pi/180;
}


void R_cal(int n_aoa, int n_tdoa, fixed_type R_noise[N_MEAS])
{
	fixed_type AOAstd_sqr = AOASTD_SQRT;
	fixed_type TDOAstd_sqr = TDOASTD_SQRT;
	n_meas1:for(int i =0; i < n_aoa+n_tdoa;i++)
	{
		if(i < n_aoa)
			R_noise[i] = AOAstd_sqr;
		else
			R_noise[i] = TDOAstd_sqr;
	}
	for(int i= n_aoa+n_tdoa; i < N_MEAS;i++){
			R_noise[i] = 1023;
	}
}
