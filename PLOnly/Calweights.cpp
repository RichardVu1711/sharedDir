#include "Calweights.h"
#define LOOP_FACTOR 32
void Calweights(fixed_type wt[NUM_PARTICLES], Mat* particle, Mat_S* obsVals,int index)
{
	Mat_S temp_cal;
	init_mat(&temp_cal,NUM_VAR,NUM_VAR);

	Mat_S temp_X;
	init_mat(&temp_X,NUM_VAR,1);

	Mat_S temp_X2;
	init_mat(&temp_X2,NUM_VAR,1);

	Mat_S temp_X3;
	init_mat(&temp_X3,1,NUM_VAR);

	Mat_S Pxx_;
	mean_X_and_Pxx(particle,&Pxx_);
//	cout<< "Pxx_:";
//	showmat_S(&Pxx_);
	msmt msmtinfo = msmt_prcs(obsVals);
//	showmat_S(&msmtinfo.z);
//	printf("Begin\n");
	// reuse resources
	// tempX2 is Hxx, tempX3 is z_cap
	init_mat(&temp_X2,SN_NUM*2,NUM_VAR);
	init_mat(&temp_X3,SN_NUM*2,1);
	fixed_type sum = 0.0;
	likelihood:for(int j =0; j < NUM_PARTICLES;j++)
	{
		getColMat_L2S(particle,j,&temp_X);
//		printf("sth\n");

//		show_mat(particle);
		temp_X2 = ObsJacobian(&temp_X,index,&msmtinfo);
//		cout<< "ObsJacobian:";
//		showmat_S(&temp_X2);
		temp_X3 = GISobs_model(&temp_X,index,&msmtinfo);
//		cout<< "GISobs_model:";
//		showmat_S(&temp_X3);
		fixed_type pzx = GISPzx(&msmtinfo,&Pxx_,&temp_X2,&temp_X3);
		double pzx_du;
		pzx_du = pzx;
//		printf("%f_%d\t",pzx_du,j);

//		std::string fol_dir = "/home/rick/Desktop/git/ESP_PF_VitisSource/ESP_PF_NoHierarchy/Data";
//		write_csv(Save_Path(fol_dir,"ObsJacobian",NUM_PARTICLES),convert_double(temp_X2.entries,6,NUM_VAR),6,NUM_VAR);
//		write_csv(Save_Path(fol_dir,"GISobs_model",NUM_PARTICLES),convert_double(temp_X3.entries,6,1),6,1);
		wt[j] = wt[j]*pzx;
		sum += wt[j];
	}
	cout << "\n";
	// normalized weights
	for(int i =0; i < NUM_PARTICLES;i++)
	{
		wt[i] = wt[i]/sum;
//		cout << wt[i] << "\t";
	}

}

void mean_X_and_Pxx(Mat* prtcls, Mat_S* Pxx_)
{
#pragma HLS PIPELINE off
	Mat_S X_avg;
	X_avg.row = NUM_VAR;
	X_avg.col = 1;

	Pxx_->row = NUM_VAR;
	Pxx_->col = NUM_VAR;
	size_t nvar_size = NUM_VAR*NUM_VAR*sizeof(fixed_type);
//	show_mat(prtcls);
//	Init_X:for(int i =0; i < NUM_VAR;i++)
//
//#pragma HLS PIPELINE
//X_avg.entries[i*NUM_VAR] = 0;
	memset(X_avg.entries, 0, nvar_size);

	// X_avg[i<2] = Sum(X[i<2]/1024)
	state_avg0:for(int i0 =0; i0 < 2*NUM_PARTICLES;i0+=NUM_PARTICLES)
	{
		fixed_type sum = 0;
		fixed_type sum_0 = 0;
		state_avg02:for(int i1 =0; i1 < NUM_PARTICLES; i1+=LOOP_FACTOR)
		{
			sum_0 = sum;
			state_avg03_add:for(int i2 =0; i2 < LOOP_FACTOR; i2++)
			{
#pragma HLS PIPELINE II=2
#pragma HLS UNROLL factor=4
						fixed_type temp = sum_0;
						sum_0 = temp +  prtcls->entries[i0+i1+i2]/(NUM_PARTICLES);
//						cout << sum_0 <<"\t";
			}
			sum = sum_0;
		}
		int step = i0/NUM_PARTICLES*NUM_VAR;
		X_avg.entries[step] = sum;
	}

	// X_avg[i>=2] = Sum(X[i>=2])/1024
	state_avg1:for(int i0 =2*NUM_PARTICLES; i0 < NUM_VAR*NUM_PARTICLES;i0+=NUM_PARTICLES)
	{
		fixed_type sum = 0;
		fixed_type sum_0 = 0;
		state_avg12:for(int i1 =0; i1 < NUM_PARTICLES; i1+=LOOP_FACTOR)
		{
			sum_0 = sum;
			state_avg13_add:for(int i2 =0; i2 < LOOP_FACTOR; i2++)
			{
#pragma HLS PIPELINE II=2
#pragma HLS UNROLL factor=4
						fixed_type temp = sum_0;
						sum_0 = temp +  prtcls->entries[i0+i1+i2] ;
			}
			sum = sum_0;
		}
		int step = i0/NUM_PARTICLES*NUM_VAR;
		X_avg.entries[step] = sum/NUM_PARTICLES;
	}
//	cout << "X_avg:\n";
//	showmat_S(&X_avg);
	avg_cal(prtcls, &X_avg, Pxx_);
	cout << "Pxx_:\n";
	showmat_S(Pxx_);
}

void avg_cal(Mat* prtcls, Mat_S* X_avg, Mat_S* Pxx_)
{
	Mat_S temp_cal;
	init_mat(&temp_cal,NUM_VAR,NUM_VAR);

	Mat_S temp_X;
	init_mat(&temp_X,NUM_VAR,1);

	Mat_S temp_X2;
	init_mat(&temp_X2,NUM_VAR,1);

	fixed_type current_pxx[NUM_VAR*NUM_VAR];
	fixed_type temp_pxx[NUM_VAR*NUM_VAR];
	fixed_type temp_pxx2[NUM_VAR*NUM_VAR];

//	for(int i =0; i < NUM_VAR*NUM_VAR;i++)
//	{
//		Pxx_->entries[i] = 0;
//	}
	size_t nvar_size = NUM_VAR*NUM_VAR*sizeof(fixed_type);

	memset(Pxx_->entries, 0, nvar_size);
	avg_cal:for(int i =0; i < NUM_PARTICLES;i++)
	{
		// current_pxx = pxx_ => prepare for the accumulator
		loadPxx(Pxx_->entries,current_pxx);
//		showmat_S(Pxx_);
		// temp_X2 = X_avg - X[i], where X[i] is the ith particle.
		// temp_X2 = X_avg - prtcls[i].
		// cout<< "X- X_avg:";
		// showmat_S(&temp_X2);
		selPrtcls(prtcls->entries,i,X_avg,&temp_X2);
//		showmat_S(&temp_X2);


		// temp_cal = (X-X_avg)*(X - X_avg)' = tempX2*tempX2'
		mulColRow(temp_X2.entries,temp_pxx);

//		 cout<< "(X-X_avg)*(X - X_avg)':";
//		 for(int i=0; i < NUM_VAR;i++)
//		 {
//			 for(int j=0; j < NUM_VAR;j++)
//				 cout << temp_pxx[i*NUM_VAR+j] << "\t";
//			 cout << "\n";
//		 }
//		 showmat_S(&temp_cal);
		// temp_cal = temp_cal/Ns before joining the sum to solve the limitation breakpoint problems.
		// Pxx_ = Pxx_ + ((X-X_)*(X-X_)')/Ns =  current_pxx + temp_pxx2/Ns
		sumAcc(current_pxx,temp_pxx,Pxx_->entries);
	}
}

void selPrtcls(fixed_type prtcls[], int col, Mat_S* X_avg, Mat_S* temp_X2)
{
//	cout << "sub\n";
	for(int k=0; k < NUM_VAR;k++)
	{

		temp_X2->entries[k*NUM_VAR] = prtcls[k*NUM_PARTICLES+col] - X_avg->entries[k*NUM_VAR];
//		cout << temp_X2->entries[k*NUM_VAR] << ", ";

	}
//	cout << "\n";
	temp_X2->row= NUM_VAR;
	temp_X2->col = 1;
}
// this is a simple matrix multiplication of A * A', where A is 13x1 matrix
void mulColRow(fixed_type inMat[], fixed_type outMat[])
{
	int k =0;
	// [13,1] x [1,13] = [13,13]
	// where outMat[i,j] = A[i,1]*A[j,1];
	matMul:for(int i =0; i < NUM_VAR;i++)
	{

		rowMul:for(int j =0; j < NUM_VAR;j++)
		{
#pragma HLS UNROLL
			outMat[k] = inMat[i*NUM_VAR]*inMat[j*NUM_VAR];
			k = k+1;
		}
	}
}

void loadPxx(fixed_type inMat[], fixed_type outMat[])
{
	loadPxx_label2:for(int i=0; i < NUM_VAR*NUM_VAR;i++)
	{
#pragma HLS UNROLL
		outMat[i] = inMat[i];
	}
}

// Accumulating the Pxx sum. inMat
// Pxx_ = Pxx_ + ((X-X_)*(X-X_)')/Ns =  current_pxx + temp_pxx2/Ns
// currPxx: Pxx_
// newCov: (X-X_)*(X-X_)'
void sumAcc(fixed_type currPxx[],fixed_type newCov[], fixed_type outMat[])
{
	fixed_type over_Ns  = 1.0/NUM_PARTICLES;
	// Pxx_ = Pxx_ + (X-X_)*(X-X_)' = temp_pxx2 + current_pxx
	sumAcc_label2:for(int i=0; i < NUM_VAR*NUM_VAR;i+=NUM_VAR)
		{
			sumAcc_label0:for(int i1=0; i1 < NUM_VAR;i1++)
			{
#pragma HLS UNROLL
				fixed_type temp = newCov[i+i1]*over_Ns;
				outMat[i+i1] = currPxx[i+i1] + temp;
			}
		}
}
