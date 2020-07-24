
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_bme280.h"

#include "ei_config_types.h"

#include "ei_device_wio_terminal.h"
#include "ei_sensors_utils.h"
#include "sensor_aq.h"

#include <Seeed_BME280.h>
#include <Wire.h>
BME280 bme280;

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

// extern sampler_callback  cb_sampler;

static float temp_data[N_TEMP_SAMPLED] = {0};

bool ei_bme280_init(void)
{
    if (!i2c_scanner(0x76))
        return false;
    bme280.init(); // use the hardware I2C
    return true;
}

bool ei_bme280_read_data(sampler_callback callback)
{

    temp_data[0] = bme280.getTemperature();        // Temperature
    temp_data[1] = (bme280.getPressure()) * 0.001; // Pressure in kPa
    temp_data[2] = bme280.getHumidity();           // Humidity

    if (callback((const void *)&temp_data[0], SIZEOF_N_TEMP_SAMPLED))
        return 1;
    return 0;
}

bool ei_bme280_setup_data_sampling(void)
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
        {{"Temp", "C"}, {"Pressure", "kPa"}, {"Humidity", "%"},
         /*{ "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps" } */},
    };

    if (!ei_bme280_init())
    {
        ei_printf("Sensor initialization failed, Please check that your device is connected properly!\n\r");
        return false;
    }

    ei_sampler_start_sampling(&ei_bme280_read_data, &payload, SIZEOF_N_TEMP_SAMPLED);
}