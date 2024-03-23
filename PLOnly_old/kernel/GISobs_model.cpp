#include "GISobs_model.h"

Mat_S GISobs_model(Mat_S* prtcl_X, int step, msmt* msmtinfo)
{


	fixed_type SNx[6] = {0,0,-1.9561,-739.8094,-383.0758,-2.3068};
	// SNx is the location of the sensor node in x-y coordinator
	// SN1_x SN1_y SN2_x SN2_y SN3_x SN3_y

	int n_aoa = msmtinfo->n_aoa;
	int n_tdoa = msmtinfo->n_tdoa;
	int aoaTag[3];
	int tdoaTag[3];

	Mat_S z_cap;
	init_mat(&z_cap,n_aoa+n_tdoa,1);

	fixed_type x = get_ele_S(prtcl_X,0,0);
	fixed_type y = get_ele_S(prtcl_X,1,0);

	fixed_type diffX[SN_NUM];
	fixed_type diffY[SN_NUM];

	fixed_type devx [SN_NUM] ={1,1,1};

	for(int i = 0; i < SN_NUM;i++)
	{
		aoaTag[i]= msmtinfo->aoaIdx[i];
		tdoaTag[i] = msmtinfo->tdoaIdx[i];
		diffX[i] = x - SNx[i*2];
		diffY[i] = y - SNx[i*2+1];

		if(diffX[i] > 30 || diffY[i] > 30 || diffX[i] < -30 || diffY[i] < -30) devx[i] =128;
	}
	// calculate AOA est
	for(int i =0; i < SN_NUM;i++)
	{
		fixed_type temp;
		ap_fixed<WORD_LENGTH,INT_LEN> b,a;
		switch(aoaTag[i])
		{
			case 1: // SN01
//				atan2(b,a) = atan2(b/a);
				b = diffX[0];
				a = diffY[0];
				temp = hls::atan2(b,a);
				temp += get_ele_S(prtcl_X,10,0);
				set_ele_S(&z_cap,i,0,temp);
				break;
			case 2: //SN02
				b = diffX[1];
				a = diffY[1];
				temp = hls::atan2(b,a);
				temp += get_ele_S(prtcl_X,11,0);
				set_ele_S(&z_cap,i,0,temp);
				break;
			case 3: //SN03
//				temp = hls::atan2(diffX[2],diffY[2]) + get_ele_S(prtcl_X,12,0);
//				set_ele_S(&z_cap,0,i,temp);
				b = diffX[2];
				a = diffY[2];
				temp = hls::atan2(b,a);
				temp += get_ele_S(prtcl_X,12,0);
				set_ele_S(&z_cap,i,0,temp);
				break;
		}

	}

	// calculate TDOA est

	// 1023 means invalid
	// 3 is maximum values for n_row or n_col
	// aoaIdx contains what node is triggered SN1 =01, SN2= 02, SN3 =03
	// tdoaIdx contains what node is triggered 12 = 01, 13= 02, 23 = 03
	int k = n_aoa;
//	std::cout << "\n" << "n_aoa " << n_aoa << "\n";
	for(int i =0; i < SN_NUM;i++)
	{
		fixed_type temp_sqrt1,temp_sqrt2,temp_sqrt3;
		ap_fixed<WORD_LENGTH,INT_LEN> b,a;
		fixed_type temp_re;
		fixed_type fs_c =FS_C;
		switch(tdoaTag[i])
		{
		// fs/c*(sqrt((X-SNx)^2+(Y-SNy)^2))
			case 1: // 1 is 12
				a = (diffX[0]/devx[0])*(diffX[0]/devx[0]) + (diffY[0]/devx[0])*(diffY[0]/devx[0]); //(X-SNx)^2+(Y-SNy)^2
				b = (diffX[1]/devx[1])*(diffX[1]/devx[1]) + (diffY[1]/devx[1])*(diffY[1]/devx[1]);
				temp_sqrt1 = hls::sqrt(a);
				temp_sqrt2 = hls::sqrt(b);
				temp_re = fs_c*(temp_sqrt1*devx[0] - temp_sqrt2*devx[1])
						+get_ele_S(prtcl_X,4,0) +get_ele_S(prtcl_X,7,0)*(step+2);
				set_ele_S(&z_cap,k++,0,temp_re);
//				std::cout<<"a "<<a<<" b "<<b<<" temp_sqrt1 "<<hls::sqrt(a)<<" temp_sqrt2 "<<hls::sqrt(b)<<" temp_re " << temp_re <<" k " <<k<<"\n";
				break;
			case 2: // 2 is 13
				a = (diffX[0]/devx[0])*(diffX[0]/devx[0]) + (diffY[0]/devx[0])*(diffY[0]/devx[0]);
				b = (diffX[2]/devx[2])*(diffX[2]/devx[2]) + (diffY[2]/devx[2])*(diffY[2]/devx[2]);
				temp_sqrt1 = hls::sqrt(a);
				temp_sqrt3 = hls::sqrt(b);
				temp_re = fs_c*(temp_sqrt1*devx[1] - temp_sqrt3*devx[2])
						+get_ele_S(prtcl_X,5,0) +get_ele_S(prtcl_X,8,0)*(step+2);
				set_ele_S(&z_cap,k++,0,temp_re);
//				std::cout<<"a "<<a<<" b "<<b<<" temp_sqrt1 "<<temp_sqrt1<<" temp_sqrt3 "<<temp_sqrt3<<" temp_re " << temp_re <<" k " <<k<<"\n";

//				std::cout<<"temp_sqrt2 "<<temp_sqrt2 << " temp_sqrt3 "<< temp_sqrt3 <<" temp_re " << temp_re <<" k " <<k-1 << " dev2 " << devx[1] << " dev3 " << devx[2]<<"\n";
				break;
			case 3: // 3 is 23
				a = (diffX[1]/devx[1])*(diffX[1]/devx[1]) + (diffY[1]/devx[1])*(diffY[1]/devx[1]);
				b = (diffX[2]/devx[2])*(diffX[2]/devx[2]) + (diffY[2]/devx[2])*(diffY[2]/devx[2]);
				temp_sqrt2 = hls::sqrt(a);
				temp_sqrt3 = hls::sqrt(b);
				temp_re = fs_c*(temp_sqrt2*devx[0] - temp_sqrt3*devx[2])
						+get_ele_S(prtcl_X,6,0) +get_ele_S(prtcl_X,9,0)*(step+2);
				set_ele_S(&z_cap,k++,0,temp_re);
//				std::cout<<"a "<<a<<" b "<<b<<" temp_sqrt2 "<<temp_sqrt2<<" temp_sqrt3 "<<temp_sqrt3<<" temp_re " << temp_re <<" k " <<k<<"\n";
//				std::cout<<"temp_sqrt3 "<<temp_sqrt3<<" temp_re " << temp_re <<" k " <<k<<"\n";
				break;
			default:// 1023 is invalid do nothing
				break;
		}

	}
	return z_cap;
}
