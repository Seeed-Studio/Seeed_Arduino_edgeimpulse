
#ifndef _EI_ULTRASONIC_H
#define _EI_ULTRASONIC_H

/* Include ----------------------------------------------------------------- */
#include "ei_sampler.h"

/** Number of axis used and sample data format */
typedef float sample_format_t;
#define N_ULTRASONIC_SAMPLED			1
#define SIZEOF_N_ULTRASONIC_SAMPLED	(sizeof(sample_format_t) * N_ULTRASONIC_SAMPLED)


/* Function prototypes ----------------------------------------------------- */
bool ei_ultrasonic_init(void);
bool ei_ultrasonic_sample_start(sampler_callback callback, float sample_interval_ms);
bool ei_ultrasonic_setup_data_sampling(void);
//bool ei_ultrasonic_read_data();
bool ei_ultrasonic_read_data(sampler_callback callback );

#endif
