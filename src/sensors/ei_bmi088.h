
#ifndef _EI_BMI088_H
#define _EI_BMI088_H

/* Include ----------------------------------------------------------------- */
#include "ei_sampler.h"

/** Number of axis used and sample data format */
typedef float sample_format_t;
#define N_GYRO_SAMPLED			6
#define SIZEOF_N_GYRO_SAMPLED	(sizeof(sample_format_t) * N_GYRO_SAMPLED)


/* Function prototypes ----------------------------------------------------- */
bool ei_bmi088_init(void);
bool ei_bmi088_sample_start(sampler_callback callback, float sample_interval_ms);
bool ei_bmi088_setup_data_sampling(void);
//bool ei_bmi088_read_data();
bool ei_bmi088_read_data(sampler_callback callback );

#endif