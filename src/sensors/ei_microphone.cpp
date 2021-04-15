
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_microphone.h"
#include "ei_config_types.h"
#include "ei_device_wio_terminal.h"
#include "sensor_aq.h"
#include "sensor_aq_mbedtls_hs256.h"
#include <Arduino.h>
#include <Seeed_Arduino_FreeRTOS.h>

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);

#define AUDIO_SAMPLING_FREQUENCY 16000
#define DEBUG 0 // Enable pin pulse during ISR

static size_t ei_write(const void*, size_t size, size_t count, EI_SENSOR_AQ_STREAM*)
{
    ei_printf("Writing: %d\r\n", count);
    return count;
}

static int ei_seek(EI_SENSOR_AQ_STREAM*, long int offset, int origin)
{
    ei_printf("Seeking: %d\r\n", offset);
    return 0;
}

/* Private variables ------------------------------------------------------- */
//static signed short *sampleBuffer;
static uint32_t block_size;
static uint32_t samples_required;
static uint32_t current_sample;
static uint32_t sample_buffer_size;
static uint32_t headerOffset = 0;

static unsigned char ei_mic_ctx_buffer[1024];
static uint8_t temp_buffer[4096];
static sensor_aq_signing_ctx_t ei_mic_signing_ctx;
static sensor_aq_mbedtls_hs256_ctx_t ei_mic_hs_ctx;

static sensor_aq_ctx ei_mic_ctx = {
    {ei_mic_ctx_buffer, 1024},
    &ei_mic_signing_ctx,
    &ei_write,
    &ei_seek,
    NULL,
};

// DMAC descriptor structure
typedef struct {
  uint16_t btctrl;
  uint16_t btcnt;
  uint32_t srcaddr;
  uint32_t dstaddr;
  uint32_t descaddr;
} dmacdescriptor ;

enum {ADC_BUF_LEN = 4096};    // Size of one of the DMA double buffers
static const int debug_pin = 1; // Toggles each DAC ISR (if DEBUG is set to 1)
volatile static bool batch_ready = 0;
volatile static uint8_t buf_idx = 0;
volatile static uint32_t batch_idx = 0;
static bool dma_init = false;
volatile bool recording = 0;

// Globals - DMA and ADC
volatile boolean results0Ready = false;
volatile boolean results1Ready = false;
int16_t adc_buf_0[ADC_BUF_LEN];    // ADC results array 0
int16_t adc_buf_1[ADC_BUF_LEN];    // ADC results array 1
volatile dmacdescriptor wrb[DMAC_CH_NUM] __attribute__ ((aligned (16)));          // Write-back DMAC descriptors
dmacdescriptor descriptor_section[DMAC_CH_NUM] __attribute__ ((aligned (16)));    // DMAC channel descriptors
dmacdescriptor descriptor __attribute__ ((aligned (16)));                         // Place holder descriptor

TaskHandle_t Handle_audioTask;

//High pass butterworth filter order=1 alpha1=0.0125 
class  FilterBuHp1
{
  public:
    FilterBuHp1()
    {
      v[0]=0.0;
    }
  private:
    float v[2];
  public:
    float step(float x) //class II 
    {
      v[0] = v[1];
      v[1] = (9.621952458291035404e-1f * x)
         + (0.92439049165820696974f * v[0]);
      return 
         (v[1] - v[0]);
    }
};

FilterBuHp1 filter;


static void finish_and_upload(char *filename, uint32_t sample_length_ms) {


    ei_printf("Done sampling, total bytes collected: %u\n", current_sample);

    ei_printf("[1/1] Uploading file to Edge Impulse...\n");

    //ei_printf("Not uploading file, not connected to WiFi. Used buffer, type=%d, from=%lu, to=%lu, sensor_name=%s, sensor_units=%s.\n",
    //            EI_INT16, 0, sample_block*4096 + headerOffset, "audio", "wav");
    ei_printf("Not uploading file, not connected to WiFi. Used buffer, from=%lu, to=%lu.\n", 0, current_sample + headerOffset);

    ei_printf("OK\n");
}


static int insert_ref(char *buffer, int hdrLength)
{
    #define EXTRA_BYTES(a)  ((a & 0x3) ? 4 - (a & 0x3) : (a & 0x03))
    const char *ref = "Ref-BINARY-i16";
    int addLength = 0;
    int padding = EXTRA_BYTES(hdrLength);

    buffer[addLength++] = 0x60 + 14 + padding;
    for(int i = 0; i < strlen(ref); i++) {
        buffer[addLength++] = *(ref + i);
    }
    for(int i = 0; i < padding; i++) {
        buffer[addLength++] = ' ';
    }

    buffer[addLength++] = 0xFF;

    return addLength;
}

static bool create_header(void)
{
    sensor_aq_init_mbedtls_hs256_context(&ei_mic_signing_ctx, &ei_mic_hs_ctx, ei_config_get_config()->sample_hmac_key);

    sensor_aq_payload_info payload = {
        EiDevice.get_id_pointer(),
        EiDevice.get_type_pointer(),
        1000.0f / static_cast<float>(AUDIO_SAMPLING_FREQUENCY),
        { { "audio", "wav" } }
    };

    int tr = sensor_aq_init(&ei_mic_ctx, &payload, NULL, true);

    if (tr != AQ_OK) {
        ei_printf("sensor_aq_init failed (%d)\n", tr);
        return false;
    }

    // then we're gonna find the last byte that is not 0x00 in the CBOR buffer.
    // That should give us the whole header
    size_t end_of_header_ix = 0;
    for (size_t ix = ei_mic_ctx.cbor_buffer.len - 1; ix >= 0; ix--) {
        if (((uint8_t*)ei_mic_ctx.cbor_buffer.ptr)[ix] != 0x0) {
            end_of_header_ix = ix;
            break;
        }
    }

    if (end_of_header_ix == 0) {
        ei_printf("Failed to find end of header\n");
        return false;
    }


    int ref_size = insert_ref(((char*)ei_mic_ctx.cbor_buffer.ptr + end_of_header_ix), end_of_header_ix);

    // and update the signature
    tr = ei_mic_ctx.signature_ctx->update(ei_mic_ctx.signature_ctx, (uint8_t*)(ei_mic_ctx.cbor_buffer.ptr + end_of_header_ix), ref_size);
    if (tr != 0) {
        ei_printf("Failed to update signature from header (%d)\n", tr);
        return false;
    }

    end_of_header_ix += ref_size;

    // Write to blockdevice
    tr = ei_sfud_fs_write_samples(ei_mic_ctx.cbor_buffer.ptr, 0, end_of_header_ix);

    if (tr != 0) {
        ei_printf("Failed to write to header blockdevice (%d)\n", tr);
        return false;
    }

    headerOffset = end_of_header_ix;

    return true;
}

static void audio_rec_callback(int16_t *buf, uint32_t buf_len) {


  // If recording re-scale and filter the samples in filled inference buffer
  if (recording) {

        uint32_t batch_samples = min(samples_required - batch_idx*buf_len, buf_len);

        for (uint32_t i = 0; i < batch_samples; i++) {
            // Convert 12-bit unsigned ADC value to 16-bit PCM (signed) audio value and apply High pass butterworth filter
            //buf[i++] = ((int16_t)buf[i] - 2048) * 16;
            buf[i++] = filter.step((buf[i] - 1024) * 16);
            }
        
        batch_idx++;
        current_sample += batch_samples; 
        batch_ready = true;
        }
}

static void audio_write_task(void* pvParameters)
{
    while (recording) {
        if (batch_ready) {
            if (buf_idx == 0) {
                ei_sfud_fs_write_samples(adc_buf_0, headerOffset + current_sample << 1, block_size*2);
                ei_mic_ctx.signature_ctx->update(ei_mic_ctx.signature_ctx, (uint8_t*)adc_buf_0, block_size*2);
            }
            else {
                ei_sfud_fs_write_samples(adc_buf_1, headerOffset + current_sample << 1, block_size*2);
                ei_mic_ctx.signature_ctx->update(ei_mic_ctx.signature_ctx, (uint8_t*)adc_buf_1, block_size*2); 
            }
        }
    }
    vTaskDelete(NULL);
}    


void DMAC_1_Handler() {

  static uint8_t count = 0;

  // Check if DMAC channel 1 has been suspended (SUSP)
  if (DMAC->Channel[1].CHINTFLAG.bit.SUSP) {

     // Debug: make pin high before copying buffer
#if DEBUG
    digitalWrite(debug_pin, HIGH);
#endif

    // Restart DMAC on channel 1 and clear SUSP interrupt flag
    DMAC->Channel[1].CHCTRLB.reg = DMAC_CHCTRLB_CMD_RESUME;
    DMAC->Channel[1].CHINTFLAG.bit.SUSP = 1;

    // See which buffer has filled up, and dump results into large buffer
    if (count) {
      buf_idx = 0;
      audio_rec_callback(adc_buf_0, ADC_BUF_LEN);
    } else {
      buf_idx = 1;
      audio_rec_callback(adc_buf_1, ADC_BUF_LEN);
    }

    // Flip to next buffer
    count = (count + 1) % 2;

    // Debug: make pin low after copying buffer
#if DEBUG
    digitalWrite(debug_pin, LOW);
#endif
  }
}

void config_dma_adc() {
  
  // Configure DMA to sample from ADC at a regular interval (triggered by timer/counter)
  DMAC->BASEADDR.reg = (uint32_t)descriptor_section;                          // Specify the location of the descriptors
  DMAC->WRBADDR.reg = (uint32_t)wrb;                                          // Specify the location of the write back descriptors
  DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);                // Enable the DMAC peripheral
  DMAC->Channel[1].CHCTRLA.reg = DMAC_CHCTRLA_TRIGSRC(TC5_DMAC_ID_OVF) |      // Set DMAC to trigger on TC5 timer overflow
                                 DMAC_CHCTRLA_TRIGACT_BURST;                  // DMAC burst transfer
                                 
  descriptor.descaddr = (uint32_t)&descriptor_section[1];                     // Set up a circular descriptor
  descriptor.srcaddr = (uint32_t)&ADC1->RESULT.reg;                           // Take the result from the ADC0 RESULT register
  descriptor.dstaddr = (uint32_t)adc_buf_0 + sizeof(uint16_t) * ADC_BUF_LEN;  // Place it in the adc_buf_0 array
  descriptor.btcnt = ADC_BUF_LEN;                                             // Beat count
  descriptor.btctrl = DMAC_BTCTRL_BEATSIZE_HWORD |                            // Beat size is HWORD (16-bits)
                      DMAC_BTCTRL_DSTINC |                                    // Increment the destination address
                      DMAC_BTCTRL_VALID |                                     // Descriptor is valid
                      DMAC_BTCTRL_BLOCKACT_SUSPEND;                           // Suspend DMAC channel 0 after block transfer
  memcpy(&descriptor_section[0], &descriptor, sizeof(descriptor));            // Copy the descriptor to the descriptor section
  
  descriptor.descaddr = (uint32_t)&descriptor_section[0];                     // Set up a circular descriptor
  descriptor.srcaddr = (uint32_t)&ADC1->RESULT.reg;                           // Take the result from the ADC0 RESULT register
  descriptor.dstaddr = (uint32_t)adc_buf_1 + sizeof(uint16_t) * ADC_BUF_LEN;  // Place it in the adc_buf_1 array
  descriptor.btcnt = ADC_BUF_LEN;                                             // Beat count
  descriptor.btctrl = DMAC_BTCTRL_BEATSIZE_HWORD |                            // Beat size is HWORD (16-bits)
                      DMAC_BTCTRL_DSTINC |                                    // Increment the destination address
                      DMAC_BTCTRL_VALID |                                     // Descriptor is valid
                      DMAC_BTCTRL_BLOCKACT_SUSPEND;                           // Suspend DMAC channel 0 after block transfer
  memcpy(&descriptor_section[1], &descriptor, sizeof(descriptor));            // Copy the descriptor to the descriptor section

  // Configure NVIC
  NVIC_SetPriority(DMAC_1_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for DMAC1 to 0 (highest)
  NVIC_EnableIRQ(DMAC_1_IRQn);         // Connect DMAC1 to Nested Vector Interrupt Controller (NVIC)

  // Activate the suspend (SUSP) interrupt on DMAC channel 1
  DMAC->Channel[1].CHINTENSET.reg = DMAC_CHINTENSET_SUSP;

  // Configure ADC
  ADC1->INPUTCTRL.bit.MUXPOS = ADC_INPUTCTRL_MUXPOS_AIN12_Val; // Set the analog input to ADC1/AIN12
  while(ADC1->SYNCBUSY.bit.INPUTCTRL);                // Wait for synchronization
  ADC1->SAMPCTRL.bit.SAMPLEN = 0x00;                  // Set max Sampling Time Length to half divided ADC clock pulse (2.66us)
  while(ADC1->SYNCBUSY.bit.SAMPCTRL);                 // Wait for synchronization 
  //ADC1->REFCTRL.bit.REFSEL = 0x1u;                   //Set reference voltage
  //while(ADC1->SYNCBUSY.bit.REFCTRL);                 // Wait for synchronization  

  ADC1->CTRLA.reg = ADC_CTRLA_PRESCALER_DIV128;       // Divide Clock ADC GCLK by 128 (48MHz/128 = 375kHz)
  ADC1->CTRLB.reg = ADC_CTRLB_RESSEL_12BIT |          // Set ADC resolution to 12 bits
                    ADC_CTRLB_FREERUN;                // Set ADC to free run mode       
  while(ADC1->SYNCBUSY.bit.CTRLB);                    // Wait for synchronization
  ADC1->CTRLA.bit.ENABLE = 1;                         // Enable the ADC
  while(ADC1->SYNCBUSY.bit.ENABLE);                   // Wait for synchronization
  ADC1->SWTRIG.bit.START = 1;                         // Initiate a software trigger to start an ADC conversion
  while(ADC1->SYNCBUSY.bit.SWTRIG);                   // Wait for synchronization

  // Enable DMA channel 1
  DMAC->Channel[1].CHCTRLA.bit.ENABLE = 1;

  // Configure Timer/Counter 5
  GCLK->PCHCTRL[TC5_GCLK_ID].reg = GCLK_PCHCTRL_CHEN |        // Enable perhipheral channel for TC5
                                   GCLK_PCHCTRL_GEN_GCLK1;    // Connect generic clock 0 at 48MHz
   
  TC5->COUNT16.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;               // Set TC5 to Match Frequency (MFRQ) mode
  TC5->COUNT16.CC[0].reg = 3000 - 1;                          // Set the trigger to 16 kHz: (4Mhz / 16000) - 1
  while (TC5->COUNT16.SYNCBUSY.bit.CC0);                      // Wait for synchronization

  // Start Timer/Counter 5
  TC5->COUNT16.CTRLA.bit.ENABLE = 1;                          // Enable the TC5 timer
  while (TC5->COUNT16.SYNCBUSY.bit.ENABLE);                   // Wait for synchronization
}


/* Public functions -------------------------------------------------------- */

bool ei_microphone_record(uint32_t sample_length_ms, uint32_t start_delay_ms, uint32_t samples_required)
{
    start_delay_ms = start_delay_ms < 2000 ? 2000 : start_delay_ms;

    uint8_t block_count = (((samples_required << 1) + 4096) / block_size) + 1;

    for (uint8_t i = 0; i < block_count; i++) {

    if (ei_sfud_fs_erase_sampledata(i, block_size) != SFUD_FS_CMD_OK) {
        ei_printf("Error erasing samples, aborting.\n");
		return false;
        }

    }

    ei_printf("Starting in %lu ms...\n", start_delay_ms);
    delay(start_delay_ms);

    create_header();

    if (!dma_init){
    config_dma_adc();
    dma_init = true;
    }

    ei_printf("Samples_required: %d \n", samples_required);

    recording = 1;
    batch_idx = 0;
    batch_ready = false;

    xTaskCreate(audio_write_task, "Data collection", block_size*2, NULL, tskIDLE_PRIORITY + 2, &Handle_audioTask);

    while (current_sample < samples_required) {  
        //ei_printf("%d\n", current_sample);
        digitalWrite(LED_BUILTIN, HIGH); 
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);    
    }

    recording = 0;
    ei_printf("Sampling finished\n");
    return true;
}

bool ei_microphone_setup_data_sampling(void)
{
    // Configure pin to toggle on DMA interrupt
    #if DEBUG
    pinMode(debug_pin, OUTPUT);
    #endif

    pinMode(LED_BUILTIN, OUTPUT);
    ei_printf("Sampling settings:\n");
    ei_printf("\tInterval: %.5f ms.\n", (float)ei_config_get_config()->sample_interval_ms);
    ei_printf("\tLength: %lu ms.\n", ei_config_get_config()->sample_length_ms);
    ei_printf("\tName: %s\n", ei_config_get_config()->sample_label);
    ei_printf("\tHMAC Key: %s\n", ei_config_get_config()->sample_hmac_key);
    char filename[256];
    int fn_r = snprintf(filename, 256, "/fs/%s", ei_config_get_config()->sample_label);
    if (fn_r <= 0) {
        ei_printf("ERR: Failed to allocate file name\n");
        return false;
    }
    ei_printf("\tFile name: %s\n", filename);


    samples_required = (uint32_t)(((float)ei_config_get_config()->sample_length_ms) / ei_config_get_config()->sample_interval_ms)*2;

    /* Round to even number of samples for word align flash write */
    if(samples_required & 1) {
        samples_required++;
    }

    current_sample = 0;

    block_size = ei_sfud_fs_get_block_size();
    ei_printf("Block_size: %d \n", block_size);

    bool r = ei_microphone_record(ei_config_get_config()->sample_length_ms, (((samples_required <<1)/ ei_sfud_fs_get_block_size()) * SFUD_FS_BLOCK_ERASE_TIME_MS), samples_required);
    if (!r) {
        ei_printf("Sampling failed\n");
        return false;
    }
    delay(10);

    uint8_t final_byte[] = { 0xff };
    int ctx_err = ei_mic_ctx.signature_ctx->update(ei_mic_ctx.signature_ctx, final_byte, 1);
    if (ctx_err != 0) {
        ei_printf("Final byte failed\n");
        return ctx_err;
    }

    ei_printf("Finalizing signature\n");
    ctx_err = ei_mic_ctx.signature_ctx->finish(ei_mic_ctx.signature_ctx, ei_mic_ctx.hash_buffer.buffer);
    if (ctx_err != 0) {
        ei_printf("Failed to finish signature (%d)\n", ctx_err);
        return false;
    }

    ei_printf("Read first page");
    int j = ei_sfud_fs_read_sample_data(temp_buffer, 0, ei_sfud_fs_get_block_size());
    if (j != 0) {
        ei_printf("Failed to read first page (%d)\n", j);
        return false;
    }

    ei_printf("Update the hash\n");
    uint8_t *hash = ei_mic_ctx.hash_buffer.buffer;
    // we have allocated twice as much for this data (because we also want to be able to represent in hex)
    // thus only loop over the first half of the bytes as the signature_ctx has written to those
    for (size_t hash_ix = 0; hash_ix < ei_mic_ctx.hash_buffer.size / 2; hash_ix++) {
        // this might seem convoluted, but snprintf() with %02x is not always supported e.g. by newlib-nano
        // we encode as hex... first ASCII char encodes top 4 bytes
        uint8_t first = (hash[hash_ix] >> 4) & 0xf;
        // second encodes lower 4 bytes
        uint8_t second = hash[hash_ix] & 0xf;

        // if 0..9 -> '0' (48) + value, if >10, then use 'a' (97) - 10 + value
        char first_c = first >= 10 ? 87 + first : 48 + first;
        char second_c = second >= 10 ? 87 + second : 48 + second;

        temp_buffer[ei_mic_ctx.signature_index + (hash_ix * 2) + 0] = first_c;
        temp_buffer[ei_mic_ctx.signature_index + (hash_ix * 2) + 1] = second_c;
    }
    delay(100);
    ei_printf("Erase first page\n");
    j = ei_sfud_fs_erase_sampledata(0, ei_sfud_fs_get_block_size());
    if (j != 0) {
        ei_printf("Failed to erase first page (%d)\n", j);
        return false;
    }
    delay(100);
    ei_printf("Write_sample\n");
    j = ei_sfud_fs_write_samples(temp_buffer, 0, ei_sfud_fs_get_block_size());

    if (j != 0) {
        ei_printf("Failed to write first page with updated hash (%d)\n", j);
        return false;
    }

    finish_and_upload(filename, ei_config_get_config()->sample_length_ms);

    return true;
}