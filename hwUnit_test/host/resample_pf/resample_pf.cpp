#include "resample_pf.h"

void resamplePF_wrap(fixed_type particle_in [NUM_VAR*NUM_PARTICLES],
					fixed_type wt[NUM_PARTICLES], fixed_type r)
{
	resample_pf(particle_in,wt,r,wt);
}

int resample_pf(fixed_type particle_in [NUM_VAR*NUM_PARTICLES],
				fixed_type weights_in[NUM_PARTICLES], fixed_type r,
				fixed_type weights_out[NUM_PARTICLES])
{
	if(r < 0) r = -r;
    fixed_type b_x=r;

    fixed_type edges[NUM_PARTICLES+1];
    fixed_type particle_out[NUM_VAR*NUM_PARTICLES];
    // Calculate cummulative sum of the weights.
    if(cumsum(weights_in,edges)) return 1;

    fixed_type start = b_x/NUM_PARTICLES;
    fixed_type step = 1.0/(NUM_PARTICLES);
    fixed_type bin_arr[NUM_PARTICLES];
    int bin_counts[NUM_PARTICLES];
    int i =0;
    bin_created(start,step,1.0,bin_arr);
    int j;


    int bin =0;
    // start create index bin array, where histogram will be performed
    // counts the number of values in bin arrray exceeding the specified bin range (cummulative sum of weights)
//    printf("idx: \n");
    for (i = 0; i < NUM_PARTICLES; i++)
    {
        bin = Which_bin(bin_arr[i],edges, NUM_PARTICLES, start);
        bin_counts[i]=bin-1;
        if(bin_counts[i] < 0) bin_counts[i] = NUM_PARTICLES-1;
    }


    // re-arrange particle based on the bin index
    // the particles with small weights will be eliminated and replaced with high-weights particles
    for (i = 0; i < NUM_PARTICLES; i++)
    {
        for (j = 0; j < NUM_VAR; j++)
        {
            particle_out[i+j*NUM_PARTICLES] = particle_in[bin_counts[i]+NUM_PARTICLES*j];
//            cout <<particle_out[i+j*NUM_PARTICLES]<<"\t";
        }
    }
//    particle_in = particle_out;
    memcpy(&particle_in[0], &particle_out[0], (unsigned) NUM_PARTICLES*NUM_VAR * sizeof(fixed_type));

    // re-arrange weights. Actually it assigns all weights to 1/Ns
    for (i = 0; i < NUM_PARTICLES; i++)
    {
        weights_out[i] = 1.0/NUM_PARTICLES;
    }
    return 0;
}


// this function determnine which bin the data belongs to.
int Which_bin(fixed_type data, fixed_type * bin_arr, int bin_count, fixed_type start_meas)
{
    int bottom = 0, top =  bin_count-1;
    int mid;
    fixed_type bin_max, bin_min;
    // binary search:
    while (bottom <= top)
    {
        mid = (bottom + top)/2;
        bin_max = bin_arr[mid];
        bin_min = (mid == 0) ? start_meas: bin_arr[mid-1];
        if (data > bin_max)
            bottom = mid+1;
        else if (data < bin_min)
            top = mid-1;
        else
            return mid;
   }
   return 0;
}

// size of boundary < NUM_PARTICLES
// generate the bin array with each maximum values at each bin.
// **NOTE**: has to free the output array as it was generated from malloc
int bin_created(fixed_type start, fixed_type step, fixed_type end, fixed_type boundary[NUM_PARTICLES])
{
    int i =0;
    fixed_type temp = start;
    // printf("%d,\t",(fixed_type)((end-start)/step));
    fixed_type instant [NUM_PARTICLES];
    for(i = 0; temp < end && i <2048;i++)
    {
        instant[i] = temp;
        temp += step;
    }
    int size_bound = i;
    for(i; i >0 ;i--)
    {
        boundary[i-1] = instant[i-1];
    }
    return size_bound;
}

// calculate the cummulative sum based on input weights. The output is stored in edges.
int cumsum(fixed_type weights[NUM_PARTICLES],fixed_type edges[NUM_PARTICLES+1])
{
    fixed_type x[NUM_PARTICLES];
    fixed_type varargin_1[NUM_PARTICLES+1];
    int k =0;
    memcpy(&x[0], &weights[0], (unsigned) NUM_PARTICLES * sizeof(fixed_type));
    for (k = 0; k < NUM_PARTICLES-1; k++) {
        x[k + 1] += x[k];
    }
    varargin_1[0] = 0.0;
    memcpy(&varargin_1[1], &x[0], (unsigned) NUM_PARTICLES * sizeof(fixed_type));
    for (k = 0; k < NUM_PARTICLES+1; k++)
    {
        edges[k] = fmin(varargin_1[k], 1.0);
    }
    return 0;
}
