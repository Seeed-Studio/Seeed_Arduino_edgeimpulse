
#ifndef _EI_BME280_H
#define _EI_BME280_H

/* Include ----------------------------------------------------------------- */
#include "ei_sampler.h"

/** Number of axis used and sample data format */
typedef float sample_format_t;
#define N_TEMP_SAMPLED			3
#define SIZEOF_N_TEMP_SAMPLED	(sizeof(sample_format_t) * N_TEMP_SAMPLED)


/* Function prototypes ----------------------------------------------------- */
bool ei_bme280_init(void);
bool ei_bme280_sample_start(sampler_callback callback, float sample_interval_ms);
bool ei_bme280_setup_data_sampling(void);
//bool ei_bme280_read_data();
bool ei_bme280_read_data(sampler_callback callback );

#endif