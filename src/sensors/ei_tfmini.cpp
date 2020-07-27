
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_tfmini.h"

#include "ei_config_types.h"


#include "ei_device_wio_terminal.h"
#include "sensor_aq.h"

#include <TFLidar.h>

TFMini SeeedTFMini;
TFLidar SeeedTFLidar(&SeeedTFMini);

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

// extern sampler_callback  cb_sampler;

static float distance_data[N_DISTANCE_SAMPLED] = {0};

bool ei_tfmini_init(void)
{   
    int timeout = 0;
    SeeedTFLidar.begin(&Serial1,115200);
    while (!Serial1.available()) {
        timeout++;
        if (timeout > 200)
            return false;
    }
    return true;
}

bool ei_tfmini_read_data(sampler_callback callback)
{
    if (SeeedTFLidar.get_frame_data())
    {
        distance_data[0] = SeeedTFLidar.get_distance(); // Distance
        distance_data[1] = SeeedTFLidar.get_strength(); // Signal Strength
    }
    
    if (callback((const void *)&distance_data[0], SIZEOF_N_DISTANCE_SAMPLED))
        return 1;
    return 0;

}

bool ei_tfmini_setup_data_sampling(void)
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
        {{"Distance", "cm"}, {"Strength", "na"},
         /*{ "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps" } */},
    };

    if (!ei_tfmini_init())
    {
        ei_printf("Sensor initialization failed, Please check that your device is connected properly!\n\r");
        return false;
    }


    ei_sampler_start_sampling(&ei_tfmini_read_data,&payload, SIZEOF_N_PRESSURE_SAMPLED);
}
