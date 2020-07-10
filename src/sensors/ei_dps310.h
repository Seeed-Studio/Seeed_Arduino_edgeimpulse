
#ifndef _EI_DPS310_H
#define _EI_DPS310_H

/* Include ----------------------------------------------------------------- */
#include "ei_sampler.h"

/** Number of axis used and sample data format */
typedef float sample_format_t;
#define N_PRESSURE_SAMPLED			2
#define SIZEOF_N_PRESSURE_SAMPLED	(sizeof(sample_format_t) * N_PRESSURE_SAMPLED)


/* Function prototypes ----------------------------------------------------- */
bool ei_dps310_init(void);
bool ei_dps310_sample_start(sampler_callback callback, float sample_interval_ms);
bool ei_dps310_setup_data_sampling(void);
//bool ei_dps310_read_data();
bool ei_dps310_read_data(sampler_callback callback );

#endif
