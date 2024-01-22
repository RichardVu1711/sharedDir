#include "env_host.h"
#include "../Calweights.h"
//#include "hls_math.h"
//using namespace hls;

void wait_for_enter() {
    std::cout << "Hit enter to continue..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
int calwPhase_execute(ESP_PF* imp, srcObj* srcX){
	srcX->src_state = CALW;
	uint8_t qIdx = srcX->srcIdx;
	cl_int err;
	switch(imp->getBlockStatus_C()){
	case READY:
		return -1;
		break;
	case EXEC:
		return 0;
		calW_execute(imp,srcX,0);
		imp->setBlockStatus_C(WAIT);
		break;
	case WAIT:
		imp->getQueue(qIdx).finish();
		if(imp->flagCheck_C(qIdx)!=0){
			imp->isClearDoneCFlag();
			// take data from here
			OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueMigrateMemObjects({imp->calW_phase.calWInfo.pzx.buf,
																				imp->calW_phase.calWInfo.zDiff.buf},
																				CL_MIGRATE_MEM_OBJECT_HOST,
																				nullptr,&imp->calW_phase.status.flag));
			write_csv("/mnt/result/pzx.csv",
					convert_double(imp->calW_phase.calWInfo.pzx.ptr,
					1,NUM_VAR*NUM_PARTICLES,-1),
					NUM_VAR,NUM_PARTICLES);
			write_csv("/mnt/result/zDiff.csv",
					convert_double(imp->calW_phase.calWInfo.zDiff.ptr,
					1,NUM_VAR*NUM_VAR,-1),
					NUM_VAR,NUM_VAR);
		}
		return 0;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

int calW_execute(ESP_PF* imp, srcObj* srcX, int index){
	// copy data into essential buffer
	cl_int err;
	uint8_t qIdx = srcX->srcIdx;
	msmt msmtinfo = msmt_prcs(srcX->obs);
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
		OCL_CHECK(err,err = imp->calW_phase.calWInfo.kCalW.setArg(3,imp->calW_phase.calWInfo.index));
		OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueMigrateMemObjects({imp->calW_phase.calWInfo.Pxx_.buf,
																		imp->calW_phase.calWInfo.R_Mat.buf,
																		imp->calW_phase.calWInfo.prtcls.buf,
																		imp->calW_phase.calWInfo.msmtinfo.buf},0));
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
				imp->calW_phase.calWInfo.pzx.ptr,
				imp->calW_phase.calWInfo.zDiff.ptr);
		imp->calW_phase.status.isDone = 1;
	}
}
int smplPhase_execute(ESP_PF* imp, srcObj* srcX){

	xrt::profile::user_range range;
	xrt::profile::user_range range_smpl("SMPL");
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
			return -1;
			break;
		case EXEC:
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
			range.start("schedule");
			MO(sigma_execute(imp,srcX,kernel_events,&data_events[0],&exec_events[0]));
			MO(rk4_execute(imp,srcX));
			MO(ESPCrtParticles_execute(imp,srcX,kernel_events,&data_events[1],&exec_events[1]));
			MO(mPxx_execute(imp,srcX,kernel_events,&data_events[0],&exec_events[0]));
			range.end();
			imp->setBlockStatus_S(WAIT);
			return 0;
			break;
		case WAIT:
			if(imp->flagCheck_S(qIdx)!=0){
				imp->isClearDoneSFlag();
				// get data
				OCL_CHECK(err, err = imp->getQueue(qIdx).enqueueMigrateMemObjects({imp->smpl_phase.axis2mmInfo.prtclsOut.buf,
																					imp->smpl_phase.mPxxInfo.mPxx.buf},
																					CL_MIGRATE_MEM_OBJECT_HOST));
				imp->getQueue(qIdx).finish();
				range_smpl.end();
				cout << "\n";
				for(int i=0; i < 3;i++){
					cout << imp->smpl_phase.axis2mmInfo.prtclsOut.ptr[i*1024] << ",";
				}
				cout << "\n";
				for(int i=0; i < 3;i++){
					cout << imp->smpl_phase.mPxxInfo.mPxx.ptr[i*13+i] << ",";
				}
				cout << "\n";
				memcpy(srcX->mPxx,
						imp->smpl_phase.axis2mmInfo.prtclsOut.ptr,
						size_pxx);
				memcpy(srcX->prtcls,
						imp->smpl_phase.axis2mmInfo.prtclsOut.ptr,
						size_large);
				write_csv("/mnt/result/ptrcls.csv",
						convert_double(imp->smpl_phase.axis2mmInfo.prtclsOut.ptr,
						1,NUM_VAR*NUM_PARTICLES,-1),
						NUM_VAR,NUM_PARTICLES);
				write_csv("/mnt/result/mpxx.csv",
						convert_double(imp->smpl_phase.mPxxInfo.mPxx.ptr,
						1,NUM_VAR*NUM_VAR,-1),
						NUM_VAR,NUM_VAR);
				imp->setBlockStatus_S(WAIT);
			}
			// check if the next block is ready to do
			if(imp->getBlockStatus_C()== READY){
				// make this block available to work with
				imp->setBlockStatus_S(READY);
				imp->setBlockStatus_C(EXEC);
				srcX->src_state = CALW;
			}
			return 0;
			break;
		default:
			return -1;
			break;
	}

	// if the code drops into here => bugs
	return 1;
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

//		cout << "rk4 data: \n";
//		for(int i=0; i < 3;i++)
//		cout << imp->smpl_phase.espCrtInfo.statePro.ptr[i] <<", ";
		// do not need to check on PS flag, as it is executed sequentially
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




