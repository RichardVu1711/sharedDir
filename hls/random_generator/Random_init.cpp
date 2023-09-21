#include "Random_init.h"
// #include "IS_dynamics_data.h"
/* Variable Definitions */
unsigned int method;
unsigned int state[625];
bool state_not_empty;
unsigned int b_method;
unsigned int b_state[2];
unsigned int c_state;
unsigned int d_state[2];

int rand_init(unsigned int seed)
{
//	 Initialize the randomness
	state_not_empty_init();
	eml_rand_init();
	eml_randn_init();
	eml_rand_mcg16807_stateful_init();
	eml_rand_shr3cong_stateful_init();
	// Set-up the seed

	method = 7U;
	b_method = seed;
	if (seed == 0) {
		b_method = 5489;
	}
	eml_rand_mt19937ar_stateful(static_cast<unsigned int>(b_method));
	eml_rand_mt19937ar_stateful(static_cast<unsigned int>(b_method));
	return 1;
}
int rand_seed(unsigned int seed)
{
	method = 7U;
	b_method = seed;
	if (seed == 0) {
		b_method = 5489;
	}
	eml_rand_mt19937ar_stateful(static_cast<unsigned int>(b_method));
	eml_rand_mt19937ar_stateful(static_cast<unsigned int>(b_method));
	return 1;
}
void state_not_empty_init(void)
{
  state_not_empty = false;
}

void eml_rand_init(void)
{
  method = 7U;
}

void eml_randn_init(void)
{
  b_method = 0U;
  b_state[0] = 362436069U;
  b_state[1] = 521288629U;
}


void eml_rand_mcg16807_stateful_init(void)
{
  c_state = 1144108930U;
}


void eml_rand_shr3cong_stateful_init(void)
{
  d_state[0] = 362436069U;
  d_state[1] = 521288629U;
}

void eml_rand_mt19937ar_stateful(unsigned int varargin_1)
{
  int mti;
  unsigned int r;
  if (!state_not_empty) {
    memset(&state[0], 0, 625U * sizeof(unsigned int));
    r = 5489U;
    for (mti = 0; mti < 623; mti++) {
      r = ((r ^ r >> 30U) * 1812433253U + mti) + 1U;
      state[mti + 1] = r;
    }
    state_not_empty = true;
  }
  r = varargin_1;
  state[0] = varargin_1;
  for (mti = 0; mti < 623; mti++) {
    r = ((r ^ r >> 30U) * 1812433253U + mti) + 1U;
    state[mti + 1] = r;
  }
  state[624] = 624U;
}

