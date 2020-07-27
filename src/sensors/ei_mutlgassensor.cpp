
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_sensors_utils.h"
#include "ei_mutlgassensor.h"

#include "ei_config_types.h"

#include "ei_device_wio_terminal.h"
#include "sensor_aq.h"

#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>
GAS_GMXXX<TwoWire> gas;

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

// extern sampler_callback  cb_sampler;

static float gas_data[N_GAS_SAMPLED] = {0};

bool ei_mutlgas_init(void)
{
    if(!i2c_scanner(0x08))
        return false;

    gas.begin(Wire, 0x08); // use the hardware I2C

    return true;
}

bool ei_mutlgas_read_data(sampler_callback callback)
{

    gas_data[0] = gas.getGM102B(); // NO2
    gas_data[1] = gas.getGM302B(); // C2H5CH
    gas_data[2] = gas.getGM502B(); // VOC
    gas_data[3] = gas.getGM702B(); // CO

    if (callback((const void *)&gas_data[0], SIZEOF_N_GAS_SAMPLED))
        return 1;
    return 0;
}

bool ei_mutlgas_setup_data_sampling(void)
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
        {{"NO2", "ppm"}, {"C2H5CH", "ppm"}, {"VOC", "ppm"}, {"CO", "ppm"},
         /*{ "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps" } */},
    };

    if (!ei_mutlgas_init())
    {
        ei_printf("Sensor initialization failed, Please check that your device is connected properly!\n\r");
        return false;
    }

    ei_sampler_start_sampling(&ei_mutlgas_read_data, &payload, SIZEOF_N_GAS_SAMPLED);

    return true;
}