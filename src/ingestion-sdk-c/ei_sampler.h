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

#ifndef _EI_SAMPLER_H
#define _EI_SAMPLER_H
#include <Seeed_Arduino_ooFreeRTOS.h>
#include "thread.hpp"
#include "ticks.hpp"

/** ei sampler callback function, call with sample data */
typedef bool (*sampler_callback)(const void *sample_buf, uint32_t byteLenght);
typedef bool (*sampler_read_data)(sampler_callback callback);

/* Function prototypes ----------------------------------------------------- */
bool ei_sampler_start_sampling(sampler_read_data read_data,void *v_ptr_payload, uint32_t sample_size);

using namespace cpp_freertos;

class ai_sampler_thread : public Thread
{
public:
    ai_sampler_thread()
        : Thread(8*1024, 3)
    {
    }
    void init(sampler_read_data arg_read_data,sampler_callback arg_callsampler,int arg_delayInMs){
        DelayInMs = arg_delayInMs;
        cb_sampler = arg_callsampler;
        // callsampler = arg_callsampler;
        read_data = arg_read_data;        
    }
protected:
  virtual void Run() {
    while (true) {
        if(read_data(cb_sampler)) break;
        Delay(Ticks::MsToTicks(DelayInMs));  
    }
  }

private:
  int DelayInMs;
  sampler_callback cb_sampler;
  sampler_read_data read_data;
};


#endif
