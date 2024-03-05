#include "env_host.h"
#include "../Calweights.h"
//#include "hls_math.h"
//using namespace hls;

void wait_for_enter() {
    std::cout << "Hit enter to continue..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
// Bugs logs
// BUGS: 26/01/26 --- added  solution
// After clearing the callback flags
// It is ready to set the callback again.
//However, as the event is not nulling
// => callback condition is still right => keep returning the values
// Hence, adding another stage WAIT2, will stop it from keep checking the callback flag
// Solution: only clear the flags before moving to the next stage


// This schedule may need to change in the future
// current mvnpdf and rsmpl are all in PS.
// Hence there is only a need of 1 wait phase in here
// in the future it may becomes exec1->wait->exec2->wait->Done!!
int rsmpPhase_execute(ESP_PF* imp, srcObj* srcX){
	srcX->src_state = RSMP;
	uint8_t qIdx = srcX->srcIdx;
	cl_int err;
//	xrt::profile::user_range range_rsmp;
//	xrt::profile::user_range range_exec;
	switch(imp->getBlockStatus_R()){
	case READY:
		return -1;
		break;
	case EXEC:
//		cout << "\nSrc " << qIdx << "is at RSMP \n";
//		range_rsmp.start("RSMP");
//		range_exec.start("PS RSMP");
		MO(mvnpdf_execute(imp,srcX));
		MO(rsmpl_execute(imp,srcX));
		MO(PFU_execute(imp,srcX));
//		range_exec.end();
		imp->setBlockStatus_R(WAIT);

		return 0;
		break;
	case WAIT:
		if(imp->flagCheck_R(qIdx)!=0){
//			range_rsmp.end();

			if(imp->getAlloMode_PFU() == PL){
				OCL_CHECK(err,
				err = imp->getQueue(qIdx).enqueueMigrateMemObjects({imp->rsmp_phase.PFUInfo.stateOut.buf,
																	imp->rsmp_phase.PFUInfo.pxxOut.buf},
																	CL_MIGRATE_MEM_OBJECT_HOST));
				imp->getQueue(qIdx).finish();
			}
//			write_csv("/mnt/result/wt.csv",
//					convert_double(imp->rsmp_phase.PFUInfo.wt.ptr,
//					1,NUM_PARTICLES,-1),
//					1,NUM_PARTICLES);
//			write_csv("/mnt/result/p_cal.csv",
//					convert_double(imp->rsmp_phase.mvnpdfInfo.p_val.ptr,
//					1,NUM_PARTICLES,-1),
//					1,NUM_PARTICLES);


			string stateOutPath = "/mnt/result/stateOut" + to_string(qIdx) +".csv";
			string pxxOutPath = "/mnt/result/pxxOut" + to_string(qIdx) +".csv";
			write_csv(stateOutPath,
					convert_double(imp->rsmp_phase.PFUInfo.stateOut.ptr,
					1,NUM_VAR,-1),
					1,NUM_VAR);
			write_csv(pxxOutPath,
					convert_double(imp->rsmp_phase.PFUInfo.pxxOut.ptr,
					1,NUM_VAR*NUM_VAR,-1),
					NUM_VAR,NUM_VAR);
			memcpy(srcX->state,
					imp->rsmp_phase.PFUInfo.stateOut.ptr,
					size_state);
			memcpy(srcX->pxx,
					imp->rsmp_phase.PFUInfo.pxxOut.ptr,
					size_pxx);
			cout << "\nThe result["<< qIdx+ 0x00<< "] is: " << srcX->state[0]
									<< ", " <<  srcX->state[1]
									<<"\n";
			cout << "\n==========================================\n";
			// make this block avaiable
			imp->setBlockStatus_R(READY);
			srcX->src_state = DONE;
			imp->isClearDoneRFlag();

		}
		return 0;
		break;
	}
	return -1;
}
int rsmpl_execute(ESP_PF* imp, srcObj* srcX){
	double re_sum =0;
	cl_int err;
	double N_eff;
//	imp->isClearDoneRFlag();
	for(int j =0; j < NUM_PARTICLES;j++){
		double temp2 = srcX->wt[j];
		re_sum += (temp2*temp2);
	}
	N_eff = 1/re_sum;

	// check if the degeneracy problems occurs
	if(N_eff <  0.5*NUM_PARTICLES){
		srcX->r_rsmpl = RNG_withSeed(0,0);
		if(imp->rsmp_phase.rsmplInfo.allo_mode == PL){
			// TODO: FINALISE this block if the resampling is conducted on PL !!!
			// if it is on PL, then use the OPENCL buffer
			memcpy(imp->rsmp_phase.rsmplInfo.prtclsIn.ptr,
					srcX->prtcls,size_large);
			memcpy(imp->rsmp_phase.rsmplInfo.wtIn.ptr,
					srcX->wt,size_wt);
			OCL_CHECK(err,err = imp->rsmp_phase.PFUInfo.kPFU.setArg(2,imp->rsmp_phase.rsmplInfo.r));

		}
		else{	//PS
			resamplePF_wrap(srcX->prtcls,
							srcX->wt,
							srcX->r_rsmpl);
//			write_csv("/mnt/result/prtcls.csv",
//					convert_double(srcX->prtcls,
//					1,NUM_VAR*NUM_PARTICLES,-1),
//					NUM_VAR,NUM_PARTICLES);
//			cout << "\nWt: " << srcX->wt[0] << "\n";
//			cout << "THERE IS A RESAMPLE, PLEASE CHECK IF THE DATA IS Correct";
		}
	}
	return 0;
}
int mvnpdf_execute(ESP_PF* imp, srcObj* srcX){
	fixed_type sum_fp =0;
	int n_obs = srcX->n_obs;
//	imp->isClearDoneRFlag();
	if(imp->getAlloMode_mvnpdf() == PL){
		// if it is on PL mode, then use the OpenCL buffer
		memcpy(imp->rsmp_phase.mvnpdfInfo.zDiff.ptr,
				imp->calW_phase.calWInfo.zDiff.ptr,
				size_zDiff);
		memcpy(imp->rsmp_phase.mvnpdfInfo.pzx.ptr,
				imp->calW_phase.calWInfo.pzx.ptr,
				size_pzx);
	}
	else{	// PS Mode
		//mvnpdf_code only can deal with
		// as it is on PS, data are taken directly from openCL buffer for calWeights
		for(int i=0; i <NUM_PARTICLES;i++){
			// Todo: copy data straight into mvnpdf_code
			double p_du = mvnpdf_code(&imp->calW_phase.calWInfo.zDiff.ptr[i*N_MEAS],
									&imp->calW_phase.calWInfo.pzx.ptr[i*N_MEAS*N_MEAS],
									n_obs);

			// update wt based on new likelihood values
			imp->rsmp_phase.mvnpdfInfo.p_val.ptr[i] = p_du;
			if(p_du!=0)	{
				srcX->wt[i] = srcX->wt[i]*imp->rsmp_phase.mvnpdfInfo.p_val.ptr[i];	// if the outlier gets here => do not make any decision
			}
			sum_fp = sum_fp + srcX->wt[i];
		}
		// ignore this caclulation if all weights are zeros
		if(sum_fp !=0){
			// normalise the weights
			for(int i =0; i < NUM_PARTICLES;i++){
				srcX->wt[i] = srcX->wt[i]/sum_fp;
			}
		}
		else{	//sum_fp ==0 => outlier
			for(int i =0; i < NUM_PARTICLES;i++){
				srcX->wt[i] = (fixed_type) 1.0/NUM_PARTICLES;
			}
		}

//		imp->rsmp_phase.status.isCallBack = 1;
//		imp->rsmp_phase.status.isDone = 1;
	}
	return 0;
}
int PFU_execute(ESP_PF* imp, srcObj* srcX){
	cl_int err;
	uint8_t qIdx = srcX->srcIdx;
//	imp->isClearDoneRFlag();

	memcpy(imp->rsmp_phase.PFUInfo.prtcls.ptr,
			srcX->prtcls,
			size_large);
	memcpy(imp->rsmp_phase.PFUInfo.wt.ptr,
			srcX->wt,
			size_wt);
	if(imp->getAlloMode_PFU() == PL){
		// schedule PFU kernel
		OCL_CHECK(err,
				err = imp->getQueue(qIdx).enqueueMigrateMemObjects({imp->rsmp_phase.PFUInfo.prtcls.buf,
																	imp->rsmp_phase.PFUInfo.wt.buf},0));
		OCL_CHECK(err,
				err = imp->getQueue(qIdx).enqueueTask(imp->rsmp_phase.PFUInfo.kPFU,
														nullptr,&imp->rsmp_phase.status.flag));
		//set callback
		imp->flagCheck_R(qIdx);
	}
	return 0;
}

int calwPhase_execute(ESP_PF* imp, srcObj* srcX){

	srcX->src_state = CALW;
	uint8_t qIdx = srcX->srcIdx;
//	xrt::profile::user_range range_smpl("CALW");

	cl_int err;
	switch(imp->getBlockStatus_C()){
	case READY:
		return -1;
		break;
	case EXEC:
//		cout << "\nSrc " << qIdx + 0x00 << "is at CALW \n";

		calW_execute(imp,srcX);
		imp->setBlockStatus_C(WAIT);
		return 0;
		break;
	case WAIT:
		if(imp->flagCheck_C(qIdx)!=0){
			// clear the flags will cause a bugs of multiple callback setup in here
			if(imp->getAlloMode_calW() == PL){			// take data from here
				OCL_CHECK(err,
				err = imp->getQueue(qIdx).enqueueMigrateMemObjects({imp->calW_phase.calWInfo.pzx.buf,
																	imp->calW_phase.calWInfo.zDiff.buf},
																	CL_MIGRATE_MEM_OBJECT_HOST));
				imp->getQueue(qIdx).finish();
			}
			// now wait for the next block available
			imp->setBlockStatus_C(WAIT2);

		}
		return 0;
		break;
	case WAIT2:
		// if the schedule job is done, the set call back should be clear
		if(imp->getBlockStatus_R()== READY){
			//make this block (calW) avaiable to work with
			imp->setBlockStatus_C(READY);
			//occupy the next block (Resampling)
			imp->setBlockStatus_R(EXEC);
			srcX->src_state = RSMP;
			imp->isClearDoneCFlag();

		}
		return 0;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

int calW_execute(ESP_PF* imp, srcObj* srcX){
	// copy data into essential buffer
	cl_int err;
	uint8_t qIdx = srcX->srcIdx;
	unsigned int index = srcX->iter_idx;
	msmt msmtinfo = msmt_prcs(&srcX->obs[index*10],
							index+1,
							srcX->cAvg,
							srcX->nAvg);

	srcX->n_aoa = msmtinfo.n_aoa;
	srcX->n_tdoa = msmtinfo.n_tdoa;
	srcX->n_obs = msmtinfo.n_aoa + msmtinfo.n_tdoa;

	R_cal(msmtinfo.n_aoa, msmtinfo.n_tdoa,imp->calW_phase.calWInfo.R_Mat.ptr);
	memcpy(imp->calW_phase.calWInfo.Pxx_.ptr,
			srcX->mPxx,
			size_pxx);
	memcpy(imp->calW_phase.calWInfo.prtcls.ptr,
			srcX->prtcls,
			size_large);
	memcpy(imp->calW_phase.calWInfo.msmtinfo.ptr,
			&msmtinfo,
			size_msmt);
	if(imp->getAlloMode_calW() == PL){
		OCL_CHECK(err,err = imp->calW_phase.calWInfo.kCalW.setArg(3,imp->calW_phase.calWInfo.index+1));
		OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueMigrateMemObjects({
																		imp->calW_phase.calWInfo.prtcls.buf,
																		imp->calW_phase.calWInfo.msmtinfo.buf,
																		imp->calW_phase.calWInfo.R_Mat.buf,
																		imp->calW_phase.calWInfo.Pxx_.buf
																		},0));
		imp->getQueue(qIdx).finish();
		// execute kernel computation
		// as this block won't be able to execute until the sigmaComp finish.
		// it is unnecessary to put event scheduling in here
		OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueTask(imp->calW_phase.calWInfo.kCalW,
												nullptr,&imp->calW_phase.status.flag));
		// set callback
		imp->flagCheck_C(qIdx);
	}
	else{
		CalPzxZdiff(imp->calW_phase.calWInfo.prtcls.ptr,
				&msmtinfo,
				imp->calW_phase.calWInfo.R_Mat.ptr,
				imp->calW_phase.calWInfo.index,
				imp->calW_phase.calWInfo.Pxx_.ptr,
				imp->calW_phase.calWInfo.zDiff.ptr,
				imp->calW_phase.calWInfo.pzx.ptr);

		imp->calW_phase.status.isCallBack = 1;
		imp->calW_phase.status.isDone = 1;
	}
	return -1;
}
int smplPhase_execute(ESP_PF* imp, srcObj* srcX){

//	xrt::profile::user_range range;
//	xrt::profile::user_range range_smpl("SMPL");
	uint8_t qIdx = srcX->srcIdx;
	srcX->src_state = SMPL;
	cl_int err;
	// even vector for controlling data migration
	std::vector<cl::Event>kernel_events;
	std::vector<cl::Event> data_events(3);	// 3 for three kernels
	std::vector<cl::Event> exec_events(4);	// 4 for three kernels
	switch(imp->getBlockStatus_S()){
		case READY:
		// only enter this block once it is ready for schedule
//			PROBE_(wait_for_enter());

			return -1;
			break;
		case EXEC:
//			PROBE_(wait_for_enter());
//			if(srcX->srcIdx == 1) 	PROBE_(wait_for_enter());
			imp->isClearDoneSFlag();
//			cout << "\nSrc " << qIdx + 0x00 << "is at SMPL \n";

			// prepare data for sigmaComp
			memcpy(imp->smpl_phase.sigmaInfo.rndIn.ptr,srcX->rndSigma,size_large);
			for(int i=0; i < NUM_VAR;i++){
				double temp = (double) srcX->pxx[i*NUM_VAR+i];
				imp->smpl_phase.sigmaInfo.pxxSqrt.ptr[i] = sqrt(temp);
			}
			// prepare data for rk4
			memcpy(imp->smpl_phase.rk4Info.stateIn.ptr,srcX->state,size_state);
			memcpy(imp->smpl_phase.rk4Info.rndrk4.ptr,srcX->rndrk4,size_rndrk4);


			//SCHEDULING
			MO(sigma_execute(imp,srcX,kernel_events,&data_events[0],&exec_events[0]));
			MO(rk4_execute(imp,srcX));
			MO(ESPCrtParticles_execute(imp,srcX,kernel_events,&data_events[1],&exec_events[1]));
			MO(mPxx_execute(imp,srcX,kernel_events,&data_events[0],&exec_events[0]));
			imp->setBlockStatus_S(WAIT);
//			PROBE_(wait_for_enter());
			return 0;
			break;
		case WAIT:
			if(imp->flagCheck_S(qIdx)!=0){
				// get data
				OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueMigrateMemObjects({imp->smpl_phase.axis2mmInfo.prtclsOut.buf,
																					imp->smpl_phase.mPxxInfo.mPxx.buf},
																					CL_MIGRATE_MEM_OBJECT_HOST));
				imp->getQueue(qIdx).finish();
//				range_smpl.end();
				memcpy(srcX->mPxx,
						imp->smpl_phase.mPxxInfo.mPxx.ptr,
						size_pxx);
				memcpy(srcX->prtcls,
						imp->smpl_phase.axis2mmInfo.prtclsOut.ptr,
						size_large);
				imp->setBlockStatus_S(WAIT2);
			}

			return 0;
			break;
		case WAIT2:
			// check if the next block is ready to do
			// if the schedule is finalise, then the callback flag should be 0
			if(imp->getBlockStatus_C()== READY){
				// make this block available to work with
				imp->setBlockStatus_S(READY);
				imp->setBlockStatus_C(EXEC);
				srcX->src_state = CALW;
				// clear flag here
				imp->isClearDoneSFlag();
				// clear event vector
				kernel_events.clear();
				data_events.clear();
				exec_events.clear();
//				return 0;
			}
			return 0;
			break;
		default:
			return -1;
			break;
	}

	// if the code drops into here => bugs
	return -1;
}

//int smpl_execut(ESP* imp, srcObj* srcX){
//	return 0;
//}

int sigma_execute(ESP_PF* imp, srcObj* srcX,
				std::vector<cl::Event> kernel_events,
				cl::Event* data_events,
				cl::Event* exec_events){
	cl_int err;
	uint8_t qIdx = srcX->srcIdx;

	/* 0 means from host*/
	if(imp->smpl_phase.sigmaInfo.allo_mode == PL){
		// execute kernel computation
		OCL_CHECK(err, err = imp->esp_control.q[qIdx].enqueueMigrateMemObjects({imp->smpl_phase.sigmaInfo.pxxSqrt.buf,
																				imp->smpl_phase.sigmaInfo.rndIn.buf},0,
																				NULL,data_events));
		kernel_events.push_back(data_events[0]);
		OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueNDRangeKernel(imp->smpl_phase.sigmaInfo.kSigma,0,1,1,
																	&kernel_events,&exec_events[0]));
		kernel_events.push_back(exec_events[0]);
	}
	else{
		// Todo: If allo  == PS, call ESPCrtParticles function, for the future implementation
		return -1;
	}


	return 0;
}

int rk4_execute(ESP_PF* imp, srcObj* srcX){
	xrt::profile::user_range range;

	if(imp->getAlloMode_rk4()==PL){
		return -1;
		// Todo: If Allo == PL, call  rk4 kernel execution, for the future implementation
	}
	else{	// PS Mode as default
		// state is the input for rk4
		// rk4 propagates the state, preparing for the extrapolation in sampling of ESP-PF
		// hence, migrate data from state into this rk4 input buffer
		range.start("rk4");
		rk4(imp->smpl_phase.rk4Info.stateIn.ptr,
				imp->smpl_phase.espCrtInfo.statePro.ptr,
				imp->smpl_phase.rk4Info.rndrk4.ptr);
		range.end();
	}
	return 0;
}

int ESPCrtParticles_execute(ESP_PF* imp, srcObj* srcX,
							std::vector<cl::Event> kernel_events,
							cl::Event* data_events,
							cl::Event* exec_events){
	cl_int err;
	uint8_t qIdx = srcX->srcIdx;

	if(imp->getAlloMode_ESPCrtParticles() == PL){
		// similar for the state_pro from rk4
		OCL_CHECK(err, err = imp->esp_control.q[qIdx].enqueueMigrateMemObjects({imp->smpl_phase.espCrtInfo.statePro.buf},0,
																				NULL,data_events));
		kernel_events.push_back(data_events[0]);

		// execute kernel computation
		// as this block won't be able to execute until the sigmaComp finish.
		// it is unnecessary to put event scheduling in here
		OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueNDRangeKernel(imp->smpl_phase.espCrtInfo.kCreate,0,1,1,
																		&kernel_events,exec_events));
		// for debugging purposes
		OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueNDRangeKernel(imp->smpl_phase.axis2mmInfo.kaxis2mm,0,1,1,
																	&kernel_events,nullptr));
		kernel_events.push_back(exec_events[0]);

	}
	else{
		//Todo: If Allo == PS, call ESPCrtParticles function, for the futher implementation
		return -1;
	}
	return 0;
}

int mPxx_execute(ESP_PF* imp, srcObj* srcX,
				std::vector<cl::Event> kernel_events,
				cl::Event* data_events,
				cl::Event* exec_events){
	cl_int err;
	uint8_t qIdx = srcX->srcIdx;

	if(imp->getAlloMode_ESPCrtParticles() == PL){
		OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueNDRangeKernel(imp->smpl_phase.mPxxInfo.kmPxx,0,1,1,
																		&kernel_events,
																		&imp->smpl_phase.status.flag));
		// set callback
		imp->flagCheck_S(qIdx);
	}
	else{
		return -1;
	}
	return 0;
}



