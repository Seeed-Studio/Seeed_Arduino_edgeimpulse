
#ifndef _EI_TFMINI_H
#define _EI_TFMINI_H

/* Include ----------------------------------------------------------------- */
#include "ei_sampler.h"

/** Number of axis used and sample data format */
typedef float sample_format_t;
#define N_DISTANCE_SAMPLED			2
#define SIZEOF_N_DISTANCE_SAMPLED	(sizeof(sample_format_t) * N_DISTANCE_SAMPLED)


/* Function prototypes ----------------------------------------------------- */
bool ei_tfmini_init(void);
bool ei_tfmini_sample_start(sampler_callback callback, float sample_interval_ms);
bool ei_tfmini_setup_data_sampling(void);
//bool ei_tfmini_read_data();
bool ei_tfmini_read_data(sampler_callback callback );

#endif
