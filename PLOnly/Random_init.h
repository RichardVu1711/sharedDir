#pragma once
// #include "rtwtypes.h"
#include <stdio.h>
#include <string.h>

extern unsigned int method;
extern unsigned int state[625];
extern bool state_not_empty;
extern unsigned int b_method;
extern unsigned int b_state[2];
extern unsigned int c_state;
extern unsigned int d_state[2];

int rand_init(unsigned int seed);
int rand_seed(unsigned int seed);

void state_not_empty_init(void);
void eml_rand_init(void);
void eml_randn_init(void);
void eml_rand_mcg16807_stateful_init(void);
void eml_rand_shr3cong_stateful_init(void);
void eml_rand_mt19937ar_stateful(unsigned int varargin_1);

