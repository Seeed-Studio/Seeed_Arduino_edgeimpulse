
#ifndef _EI_LIGHT_H
#define _EI_LIGHT_H

/* Include ----------------------------------------------------------------- */
#include "ei_sampler.h"

/** Number of axis used and sample data format */
typedef float sample_format_t;
#define N_LIGHT_SAMPLED			1
#define SIZEOF_N_LIGHT_SAMPLED	(sizeof(sample_format_t) * N_LIGHT_SAMPLED)


/* Function prototypes ----------------------------------------------------- */
bool ei_light_init(void);
bool ei_light_sample_start(sampler_callback callback, float sample_interval_ms);
bool ei_light_setup_data_sampling(void);
bool ei_light_read_data(sampler_callback callback );

#endif
