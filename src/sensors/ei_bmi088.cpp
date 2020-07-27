
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_bmi088.h"

#include "ei_config_types.h"

#include "ei_device_wio_terminal.h"
#include "ei_sensors_utils.h"
#include "sensor_aq.h"

#include <BMI088.h>

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

// extern sampler_callback  cb_sampler;

static float gyro_data[N_GYRO_SAMPLED] = {0};

bool ei_bmi088_init(void)
{
    Wire.begin();
    if (!i2c_scanner(0x19))
        return false;
    if (bmi088.isConnection()) {
        bmi088.initialize();
        return true;
    }
}

bool ei_bmi088_read_data(sampler_callback callback)
{
    bmi088.getAcceleration(&gyro_data[0], &gyro_data[1], &gyro_data[2]); // ax ay az
    bmi088.getGyroscope(&gyro_data[3], &gyro_data[4], &gyro_data[5]); // gx gy gz

    if (callback((const void *)&gyro_data[0], SIZEOF_N_GYRO_SAMPLED))
        return 1;
    return 0;
}

bool ei_bmi088_setup_data_sampling(void)
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
        {{"accX", "m/s2"}, {"accY", "m/s2"}, {"accZ", "m/s2"},{"gyrX", "degrees/s"},{"gyrY", "degrees/s"},{"gyrZ", "degrees/s"},
         /*{ "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps" } */},
    };

    if (!ei_bmi088_init())
    {
        ei_printf("Sensor initialization failed, Please check that your device is connected properly!\n\r");
        return false;
    }

    ei_sampler_start_sampling(&ei_bmi088_read_data,&payload, SIZEOF_N_GYRO_SAMPLED);
}