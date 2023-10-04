#include "rk4.h"

void rk4(Mat_S* R_input,Mat_S* X,fixed_type rnd_data[4*NUM_VAR])
{

    int i,j;
    fixed_type xh,h,h6,k_3 [NUM_VAR],k_2 [NUM_VAR],yt[NUM_VAR],yout[NUM_VAR],k_1[NUM_VAR],y[NUM_VAR];

    fixed_type t_step_local = 1;
    fixed_type t_span[2] = {0,1};
    h=t_step_local;
    fixed_type temp = 6.0;
    h6=t_step_local/temp;
    // calculated the number of the loops:
    int loop_num = (int) (t_span[1] - t_span[0])/t_step_local;
    // create a t_sim array to monitor the interval time
    fixed_type t_sim [2];
    // t_sim = vector(1,loop_num);
    t_sim[0] = t_span[0];
    init_tsim: for(i = 1; i < loop_num;i++)
    {
        t_sim[i] = t_sim[i-1] + h;
    }

//    // copy the first array of the input
    copy_input: for(i = 0;i < NUM_VAR; i++)
    {
        y[i] = get_ele_S(R_input,i,0);
    }
    // the time interval is requires to determine how many rk4 will be computed
    fixed_type rnd_epoch[NUM_VAR];
    // the original algorithm in "Numerical Receipe in C" will save more space and computation time

     rk_loop: for(j = 1; j <= loop_num; j ++){                                // expecting 1 loop only
    	 	for(int i=0; i < NUM_VAR;i++)	rnd_epoch[i] = rnd_data[i];
            IS_dynamics(y,k_1,rnd_epoch);                                     //First step.
            First_step:for (i=0;i<NUM_VAR;i++)
            {
                yt[i]=y[i]+h*1/2*k_1[i];
            }

            for(int i=0; i < NUM_VAR;i++)	rnd_epoch[i] = rnd_data[NUM_VAR+i];
            IS_dynamics(yt,k_2,rnd_epoch);                           //Second step.
            Second_step: for (i=0;i<NUM_VAR;i++)
            {
                yt[i]=y[i]+h*1/2*k_2[i];
            }

            for(int i=0; i < NUM_VAR;i++)	rnd_epoch[i] = rnd_data[NUM_VAR*2+i];
            IS_dynamics(yt,k_3,rnd_epoch);                           //Third step.
            Third_step:for (i=0;i<NUM_VAR;i++)
            {
                yt[i]=y[i]+h*k_3[i];                                // k_2 will be saved as the k_4 at the end
                k_3[i] += k_2[i];                                   // k_3 will be saved as the sum of k_3 and k_2 to save space at the end
            }

            for(int i=0; i < NUM_VAR;i++)	rnd_epoch[i] = rnd_data[NUM_VAR*3+i];
            IS_dynamics(yt,k_2,rnd_epoch);                          //Fourth step.
            Fourth_step:for (i=0;i<NUM_VAR;i++)                              //Accumulate increments with proper
            {
                yout[i]=y[i]+h6*(k_1[i]+k_2[i]+2*k_3[i]);    //weights.
                set_ele_S(X,i,0,yout[i]);

            }
        }
}
