
#ifndef _EI_SCD30_H
#define _EI_SCD30_H

/* Include ----------------------------------------------------------------- */
#include "ei_sampler.h"

/** Number of axis used and sample data format */
typedef float sample_format_t;
#define N_SCD30_SAMPLED			3
#define SIZEOF_N_SCD30_SAMPLED	(sizeof(sample_format_t) * N_SCD30_SAMPLED)


/* Function prototypes ----------------------------------------------------- */
bool ei_scd30_init(void);
bool ei_scd30_sample_start(sampler_callback callback, float sample_interval_ms);
bool ei_scd30_setup_data_sampling(void);
//bool ei_scd30_read_data();
bool ei_scd30_read_data(sampler_callback callback );

#endif