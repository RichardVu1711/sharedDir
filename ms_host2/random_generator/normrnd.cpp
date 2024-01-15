#include "normrnd.h"
using namespace std;

fixed_type normrnd (fixed_type mu, fixed_type sigma, fixed_type i_run)
{

//	fixed_type temp =(fixed_type) std::rand()/RAND_MAX;
//	fixed_type temp =my_rand();
	double temp;
	temp =  RNG_withSeed(0,i_run);
	fixed_type temp_fp = temp;
	fixed_type r = temp_fp * sigma + mu;
    return r;
}
