
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_inertialsensor.h"

#include "ei_config_types.h"

// #include <Arduino_LSM9DS1.h>

#include "ei_device_wio_terminal.h"
#include "sensor_aq.h"

#include "LIS3DHTR.h"
#include <Wire.h>
LIS3DHTR<TwoWire> LIS; //IIC

/* Constant defines -------------------------------------------------------- */
#define CONVERT_G_TO_MS2 9.80665f

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

// extern sampler_callback  cb_sampler;

static float imu_data[N_AXIS_SAMPLED] = {0};

bool ei_inertial_init(void)
{
    LIS.begin(Wire1); //IIC init
    if(!LIS)
        return false; 
    delay(100);
    LIS.setOutputDataRate(LIS3DHTR_DATARATE_200HZ);
    LIS.setHighSolution(true); //High solution enable
    delay(50);
    return true;
}

bool ei_inertial_read_data(sampler_callback callback)
{

    if (LIS.available())
    {
        LIS.getAcceleration(&imu_data[0], &imu_data[1], &imu_data[2]);
        imu_data[0] *= CONVERT_G_TO_MS2;
        imu_data[1] *= CONVERT_G_TO_MS2;
        imu_data[2] *= CONVERT_G_TO_MS2;
    }
    if (callback((const void *)&imu_data[0], SIZEOF_N_AXIS_SAMPLED))
        return 1;
    return 0;
}

// bool ei_inertial_sample_start(sampler_callback callsampler, float sample_interval_ms)
// {
// 	cb_sampler = callsampler;

//     inertion_thread.start(callback(&intertion_queue, &EventQueue::dispatch_forever));
//     sample_rate.attach(intertion_queue.event(&ei_inertial_read_data), (sample_interval_ms / 1000.f));
//     return true;
// }

bool ei_inertial_setup_data_sampling(void)
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
        // The axes which you'll use. The units field needs to comply to SenML units (see https://www.iana.org/assignments/senml/senml.xhtml)
        {{"accX", "m/s2"}, {"accY", "m/s2"}, {"accZ", "m/s2"},
         /*{ "gyrX", "dps" }, { "gyrY", "dps" }, { "gyrZ", "dps" } */},
    };

    if(!ei_inertial_init())
    {
        ei_printf("Sensor initialization failed, Please check that your device is connected properly!\n\r");
        return false;
    }

    ei_sampler_start_sampling(&ei_inertial_read_data, &payload, SIZEOF_N_AXIS_SAMPLED);
}