/* Edge Impulse ingestion SDK
 * Copyright (c) 2020 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Include ----------------------------------------------------------------- */
#include "ei_device_wio_terminal.h"

#include <stdio.h>
#include "Arduino.h"
/* Constants --------------------------------------------------------------- */
/** Memory location for the arduino device address */
#define DEVICE_ID_LSB_ADDR  ((uint32_t)0x41002018)
#define DEVICE_ID_MSB_ADDR  ((uint32_t)0x04)

/** Max size for device id array */
#define DEVICE_ID_MAX_SIZE  32

/** Sensors */
typedef enum
{
    ACCELEROMETER = 0,
    MICROPHONE

}used_sensors_t;

#define EDGE_STRINGIZE_(x) #x
#define EDGE_STRINGIZE(x) EDGE_STRINGIZE_(x)

/** Device type */
static const char *ei_device_type = EDGE_STRINGIZE(TARGET_NAME);

/** Device id array */
static char ei_device_id[DEVICE_ID_MAX_SIZE];

/** Device object, for this class only 1 object should exist */
EiDeviceWioTerminal EiDevice;

/* Private function declarations ------------------------------------------- */
static int get_id_c(uint8_t out_buffer[32], size_t *out_size);
static int get_type_c(uint8_t out_buffer[32], size_t *out_size);
static bool get_wifi_connection_status_c(void);
static bool get_wifi_present_status_c(void);

/* Public functions -------------------------------------------------------- */

EiDeviceWioTerminal::EiDeviceWioTerminal(void)
{
    uint32_t *id_msb = (uint32_t *)DEVICE_ID_MSB_ADDR;
    uint32_t *id_lsb = (uint32_t *)DEVICE_ID_LSB_ADDR;
    /* Setup device ID */
    snprintf(&ei_device_id[0], DEVICE_ID_MAX_SIZE, "%02X:%02X:%02X:%02X:%02X:%02X"
        ,(*id_msb >> 8) & 0xFF
        ,(*id_msb >> 0) & 0xFF
        ,(*id_lsb >> 24)& 0xFF
        ,(*id_lsb >> 16)& 0xFF
        ,(*id_lsb >> 8) & 0xFF
        ,(*id_lsb >> 0) & 0xFF
        );

}

/**
 * @brief      For the device ID, the BLE mac address is used.
 *             The mac address string is copied to the out_buffer.
 *
 * @param      out_buffer  Destination array for id string
 * @param      out_size    Length of id string
 *
 * @return     0
 */
int EiDeviceWioTerminal::get_id(uint8_t out_buffer[32], size_t *out_size)
{
    return get_id_c(out_buffer, out_size);
}

/**
 * @brief      Gets the identifier pointer.
 *
 * @return     The identifier pointer.
 */
const char *EiDeviceWioTerminal::get_id_pointer(void)
{
    return (const char *)ei_device_id;
}

/**
 * @brief      Copy device type in out_buffer & update out_size
 *
 * @param      out_buffer  Destination array for device type string
 * @param      out_size    Length of string
 *
 * @return     -1 if device type string exceeds out_buffer
 */
int EiDeviceWioTerminal::get_type(uint8_t out_buffer[32], size_t *out_size)
{
    return get_type_c(out_buffer, out_size);
}

/**
 * @brief      Gets the type pointer.
 *
 * @return     The type pointer.
 */
const char *EiDeviceWioTerminal::get_type_pointer(void)
{
    return (const char *)ei_device_type;
}

/**
 * @brief      No Wifi available for device.
 *
 * @return     Always return false
 */
bool EiDeviceWioTerminal::get_wifi_connection_status(void)
{
    return false;
}

/**
 * @brief      No Wifi available for device.
 *
 * @return     Always return false
 */
bool EiDeviceWioTerminal::get_wifi_present_status(void)
{
    return false;
}

/**
 * @brief      Create sensor list with sensor specs
 *             The studio and daemon require this list
 * @param      sensor_list       Place pointer to sensor list
 * @param      sensor_list_size  Write number of sensors here
 *
 * @return     False if all went ok
 */
bool EiDeviceWioTerminal::get_sensor_list(const ei_device_sensor_t **sensor_list, size_t *sensor_list_size)
{
    // /* Calculate number of bytes available on flash for sampling, reserve 1 block for header + overhead */
    // uint32_t available_bytes = (ei_nano_fs_get_n_available_sample_blocks()-1) * ei_nano_fs_get_block_size();
     uint32_t available_bytes = ei_sfud_fs_get_n_available_sample_blocks() - 1;

    sensors[ACCELEROMETER].name = "Built-in accelerometer";
    sensors[ACCELEROMETER].start_sampling_cb = &ei_inertial_setup_data_sampling;
    sensors[ACCELEROMETER].max_sample_length_s = available_bytes / (100 * SIZEOF_N_AXIS_SAMPLED);
    sensors[ACCELEROMETER].frequencies[0] = 62.5f;
    sensors[ACCELEROMETER].frequencies[1] = 100.0f;

    // sensors[MICROPHONE].name = "Built-in microphone";
    // sensors[MICROPHONE].start_sampling_cb = &ei_microphone_sample_start;
    // sensors[MICROPHONE].max_sample_length_s = available_bytes / (16000 * 2);
    // sensors[MICROPHONE].frequencies[0] = 16000.0f;

    *sensor_list      = sensors;
    *sensor_list_size = EI_DEVICE_N_SENSORS;

    return false;
}

/**
 * @brief      Get a C callback for the get_id method
 *
 * @return     Pointer to c get function
 */
c_callback EiDeviceWioTerminal::get_id_function(void)
{
    return &get_id_c;
}

/**
 * @brief      Get a C callback for the get_type method
 *
 * @return     Pointer to c get function
 */
c_callback EiDeviceWioTerminal::get_type_function(void)
{
    return &get_type_c;
}

/**
 * @brief      Get a C callback for the get_wifi_connection_status method
 *
 * @return     Pointer to c get function
 */
c_callback_status EiDeviceWioTerminal::get_wifi_connection_status_function(void)
{
    return &get_wifi_connection_status_c;
}

/**
 * @brief      Get a C callback for the wifi present method
 *
 * @return     The wifi present status function.
 */
c_callback_status EiDeviceWioTerminal::get_wifi_present_status_function(void)
{
    return &get_wifi_present_status_c;
}

/**
 * @brief      Printf function uses vsnprintf and output using Arduino Serial
 *
 * @param[in]  format     Variable argument list
 */
void ei_printf(const char *format, ...) {
    char print_buf[1024] = { 0 };

    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);

    if (r > 0) {
        Serial.write(print_buf);
    }
}
extern "C"{
    void cm_printf(const char *format, ...) {
        char print_buf[1024] = { 0 };

        va_list args;
        va_start(args, format);
        int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
        va_end(args);

        if (r > 0) {
            Serial.write(print_buf);
        }
    }    
}
/**
 * @brief      Write serial data with length to Serial output
 *
 * @param      data    The data
 * @param[in]  length  The length
 */
void ei_write_string(char *data, int length) {
    Serial.write(data, length);
}

/**
 * @brief      Get Arduino serial object
 *
 * @return     pointer to Serial
 */
Stream* ei_get_serial() {
    return &Serial;
}

/**
 * @brief      Check if new serial data is available
 *
 * @return     Returns number of available bytes
 */
int ei_get_serial_available(void) {
    return Serial.available();
}

/**
 * @brief      Get next available byte
 *
 * @return     byte
 */
char ei_get_serial_byte(void) {
    return Serial.read();
}

/* Private functions ------------------------------------------------------- */

static int get_id_c(uint8_t out_buffer[32], size_t *out_size)
{
    size_t length = strlen(ei_device_id);

    if(length < 32) {
        memcpy(out_buffer, ei_device_id, length);

        *out_size = length;
        return 0;
    }

    else {
        *out_size = 0;
        return -1;
    }
}

static int get_type_c(uint8_t out_buffer[32], size_t *out_size)
{
    size_t length = strlen(ei_device_type);

    if(length < 32) {
        memcpy(out_buffer, ei_device_type, length);

        *out_size = length;
        return 0;
    }

    else {
        *out_size = 0;
        return -1;
    }
}

static bool get_wifi_connection_status_c(void)
{
    return false;
}

static bool get_wifi_present_status_c(void)
{
    return false;
}