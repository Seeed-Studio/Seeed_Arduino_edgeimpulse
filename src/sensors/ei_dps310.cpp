
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_dps310.h"

#include "ei_config_types.h"


#include "ei_device_wio_terminal.h"
#include "ei_sensors_utils.h"
#include "sensor_aq.h"

#include <Dps310.h>
#include <Wire.h>
Dps310 Dps310PressureSensor = Dps310();



extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

// extern sampler_callback  cb_sampler;

static float pressure_data[N_PRESSURE_SAMPLED] = {0};
float temperature;
float pressure;

bool ei_dps310_init(void)
{
    if(!i2c_scanner(0x77))
        return false;
    Dps310PressureSensor.begin(Wire);
    return true;
}

bool ei_dps310_read_data(sampler_callback callback)
{

    Dps310PressureSensor.measureTempOnce(temperature);
    pressure_data[0] = temperature; // Temperature
    Dps310PressureSensor.measurePressureOnce(pressure);
    pressure_data[1] = pressure*0.001; // Pressure in kPa

    if (callback((const void *)&pressure_data[0], SIZEOF_N_PRESSURE_SAMPLED))
        return 1;
    return 0;
}

bool ei_dps310_setup_data_sampling(void)
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
        {{"Temp", "C"}, {"Pressure", "kPa"},
         /*{ "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps" } */},
    };

    if (!ei_dps310_init())
    {
        ei_printf("Sensor initialization failed, Please check that your device is connected properly!\n\r");
        return false;
    }

    ei_sampler_start_sampling(&ei_dps310_read_data,&payload, SIZEOF_N_PRESSURE_SAMPLED);
}
