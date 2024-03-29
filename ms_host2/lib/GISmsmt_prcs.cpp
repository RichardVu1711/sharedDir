#include "../lib/GISmsmt_prcs.h"



// msmt information

// 1023 means invalid
// 3 is maximum values for n_row or n_col
// aoaIdx contains what node is triggered SN1 =01, SN2= 02, SN3 =03
// tdoaIdx contains what node is triggered 12 = 01, 13= 02, 23 = 03

msmt msmt_prcs(fixed_type obsVals[])
{
	// obsVals dimension is 1x10
	//msmt.z dimension is 6x1
	msmt msmtinfo;

	msmtinfo.n_aoa= 0;
	msmtinfo.n_tdoa= 0;
	// initialise valid flag
	for(int i=0; i < N_MEAS;i++)
	{
		msmtinfo.validIdx[i] = 0;
	}
	int k = 0;
	AOA_cal:for(int i =0; i < SN_NUM;i++)
	{
		fixed_type AOA = obsVals[i];

//		fixed_type AOA = obsVals->entries[i];
//		fixed_type TDOA = obsVals.entries[i+3];
		if(AOA !=1023)
		{
			if(AOA> 180) AOA = -(360 - AOA);
			AOA = deg2Rad(AOA);
			// set AOA values to entries
			msmtinfo.z[k++] = AOA;
			msmtinfo.n_aoa++;
			msmtinfo.aoaIdx[i] = i+1;
			// flag this data is valid
			msmtinfo.validIdx[i] = 1;
		}
		else
		{
			// receive invalid values
			// flag this data is invalid
			msmtinfo.validIdx[i] = 1;
			msmtinfo.aoaIdx[i] = 1023;
		}
	}
	for(int i =0; i < SN_NUM;i++)
	{
		fixed_type TDOA = obsVals[i+3];
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
//	cout << "Valid Data Idx: \n";
//	for(int i=0; i < N_MEAS;i++){
//		cout << msmtinfo.validIdx[i] << ", ";
//	}

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
//	int n_aoa = n_aoa;
//	int n_tdoa = n_tdoa;
	n_meas1:for(int i =0; i < n_aoa+n_tdoa;i++)
	{
		if(i < n_aoa)
			R_noise[i*N_MEAS+i] = AOAstd_sqr;
		else
			R_noise[i*N_MEAS+i] = TDOAstd_sqr;
	}
	for(int i= n_aoa+n_tdoa; i < N_MEAS;i++){
		R_noise[i*N_MEAS+i] = 1023;
	}
}

