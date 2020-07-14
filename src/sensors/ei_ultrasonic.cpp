
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_ultrasonic.h"

#include "ei_config_types.h"


#include "ei_device_wio_terminal.h"
#include "sensor_aq.h"

#include <Ultrasonic.h>

Ultrasonic ultrasonic(0); // Grove DO/D1 Port of Wio Terminal

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

// extern sampler_callback  cb_sampler;

static float ultrasonic_data[N_ULTRASONIC_SAMPLED] = {0};

bool ei_ultrasonic_init(void)
{
    // Nothing
}

bool ei_ultrasonic_read_data(sampler_callback callback)
{
    ultrasonic_data[0] = ultrasonic.MeasureInCentimeters(); // Distance in cm
    // delay(20); // Short delay to avoid huge data sets
    
    if (callback((const void *)&ultrasonic_data[0], SIZEOF_N_ULTRASONIC_SAMPLED))
        return 1;
    return 0;

}

bool ei_ultrasonic_setup_data_sampling(void)
{

    if (ei_config_get_config()->sample_interval_ms < 10.0f)
    {
        ei_config_set_sample_interval(10.0f);
    }

    sensor_aq_payload_info payload = {
        // Unique device ID (optional), set this to e.g. MAC address or device EUI **if** your device has one
        EiDevice.get_id_pointer(),
        // Device type (required), use the same device type for similar devices
        EiDevice.get_type_pointer(),
        // How often new data is sampled in ms. (100Hz = every 10 ms.)
        ei_config_get_config()->sample_interval_ms,
        {{"Distance", "cm"},
         /*{ "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps" } */},
    };

    ei_sampler_start_sampling(&ei_ultrasonic_read_data,&payload, SIZEOF_N_ULTRASONIC_SAMPLED);
}
