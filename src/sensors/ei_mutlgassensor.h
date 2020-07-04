
#ifndef _EI_MUTLGASSENSOR_H
#define _EI_MUTLGASSENSOR_H

/* Include ----------------------------------------------------------------- */
#include "ei_sampler.h"

/** Number of axis used and sample data format */
typedef float sample_format_t;
#define N_GAS_SAMPLED			4
#define SIZEOF_N_GAS_SAMPLED	(sizeof(sample_format_t) * N_GAS_SAMPLED)


/* Function prototypes ----------------------------------------------------- */
bool ei_mutlgas_init(void);
bool ei_mutlgas_sample_start(sampler_callback callback, float sample_interval_ms);
bool ei_mutlgas_setup_data_sampling(void);
//bool ei_mutlgas_read_data();
bool ei_mutlgas_read_data(sampler_callback callback );

#endif