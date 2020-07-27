
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_scd30.h"

#include "ei_config_types.h"


#include "ei_device_wio_terminal.h"
#include "ei_sensors_utils.h"
#include "sensor_aq.h"

#include <SCD30.h>

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

// extern sampler_callback  cb_sampler;

static float scd30_data[N_SCD30_SAMPLED] = {0};

bool ei_scd30_init(void)
{
    if(!i2c_scanner(0x61))
        return false;
    scd30.initialize(); // use the hardware I2C
    return true;
}

bool ei_scd30_read_data(sampler_callback callback)
{

    if (scd30.isAvailable()) {
        scd30.getCarbonDioxideConcentration(scd30_data);
    }

    if (callback((const void *)&scd30_data[0], SIZEOF_N_SCD30_SAMPLED))
        return 1;
    return 0;
}

bool ei_scd30_setup_data_sampling(void)
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
        {{"CO2", "ppm"}, {"Temperature", "C"}, {"Humidity", "%"},
         /*{ "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps" } */},
    };

    if (!ei_scd30_init())
    {
        ei_printf("Sensor initialization failed, Please check that your device is connected properly!\n\r");
        return false;
    }

    ei_sampler_start_sampling(&ei_scd30_read_data,&payload, SIZEOF_N_TEMP_SAMPLED);
}