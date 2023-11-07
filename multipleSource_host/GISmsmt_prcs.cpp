#include "GISmsmt_prcs.h"


// msmt information

// 1023 means invalid
// 3 is maximum values for n_row or n_col
// aoaIdx contains what node is triggered SN1 =01, SN2= 02, SN3 =03
// tdoaIdx contains what node is triggered 12 = 01, 13= 02, 23 = 03

msmt msmt_prcs(fixed_type obsVals[10])
{
	// obsVals dimension is 1x10
	//msmt.z dimension is 6x1
	msmt msmtinfo;
	init_mat(&(msmtinfo.z),SN_NUM*2,1);

	msmtinfo.n_aoa= 0;
	msmtinfo.n_tdoa= 0;

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
			set_ele_S(&(msmtinfo.z),k++,0,AOA);
			msmtinfo.n_aoa++;
			msmtinfo.aoaIdx[i] = i+1;
		}
		else
		{
			// receive invalid values
			msmtinfo.aoaIdx[i] = 1023;
		}
	}
//		fixed_type TDOA =get_ele_S(obsVals,0,i+3);
	for(int i =0; i < SN_NUM;i++)
	{
		fixed_type TDOA = obsVals[i+3];
		if(TDOA !=1023)
		{
			// set TDOA values to entries
			set_ele_S(&(msmtinfo.z),k++,0,TDOA);
			msmtinfo.n_tdoa++;
			msmtinfo.tdoaIdx[i] = i+1;
		}
		else
		{
			msmtinfo.tdoaIdx[i] = 1023;
		}
	}

	return msmtinfo;
}

fixed_type deg2Rad(fixed_type deg)
{
	fixed_type pi = M_PI;
	return deg*pi/180;
}
