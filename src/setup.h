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

#ifndef _EI_ARDUINO_WIO_TERMINAL_SETUP_H_
#define _EI_ARDUINO_WIO_TERMINAL_SETUP_H_
#include "Arduino.h"
#include "Seeed_Arduino_FreeRTOS.h"
#include "Seeed_Arduino_ooFreeRTOS.h"
#include "Seeed_mbedtls.h"
#include "task.h"
#include "thread.hpp"
#include "ticks.hpp"
#include "message.hpp"

using namespace cpp_freertos;

extern void ei_printf(const char *format, ...);
extern void ei_write_string(char *data, int length);
extern Stream* ei_get_serial();
int ei_get_serial_available(void);
char ei_get_serial_byte(void);


void print_memory_info();
void ei_main();

#endif // _EI_ARDUINO_WIO_TERMINAL_SETUP_H_
