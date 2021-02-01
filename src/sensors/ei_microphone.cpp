
/* Include ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdlib.h>

#include "ei_microphone.h"
#include "ei_config_types.h"
#include "ei_device_wio_terminal.h"
#include "sensor_aq.h"
#include "sensor_aq_mbedtls_hs256.h"
#include <Arduino.h>

extern void ei_printf(const char *format, ...);
extern ei_config_t *ei_config_get_config();
extern EI_CONFIG_ERROR ei_config_set_sample_interval(float interval);


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
static int8_t temp_buffer[4096];
static sensor_aq_signing_ctx_t ei_mic_signing_ctx;
static sensor_aq_mbedtls_hs256_ctx_t ei_mic_hs_ctx;
static sensor_aq_ctx ei_mic_ctx = {
    { ei_mic_ctx_buffer, 1024 },
    &ei_mic_signing_ctx,
    &ei_write,
    &ei_seek,
    NULL,
};



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
        1000.0f / static_cast<float>(8000),
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


/* Public functions -------------------------------------------------------- */

bool ei_microphone_record(uint32_t sample_length_ms, uint32_t start_delay_ms, uint32_t samples_required)
{
    start_delay_ms = start_delay_ms < 2000 ? 2000 : start_delay_ms;

    if(ei_sfud_fs_erase_sampledata(0, (samples_required) + 4096) != SFUD_FS_CMD_OK)
		return false;

    ei_printf("Starting in %lu ms... (or until all flash was erased)\n", start_delay_ms);

    pinMode(WIO_MIC, INPUT);
    delay(start_delay_ms);

    create_header();

    ei_printf("Sampling...\n");

    unsigned int sampling_period_us = round(600000 * (1.0 / 16000));
    ei_printf("Samples_required: %d \n", samples_required);

    while (current_sample < (samples_required)) {
        ei_printf("Current_sample: %d \n", current_sample);
        for (uint32_t i = 0; i < block_size; i++) {
            temp_buffer[i] = map(analogRead(WIO_MIC), 0, 1023, -128, 127);
            delayMicroseconds(sampling_period_us);
        }
        current_sample += block_size;
        ei_sfud_fs_write_samples((const void *)temp_buffer, headerOffset + current_sample, block_size);
        ei_mic_ctx.signature_ctx->update(ei_mic_ctx.signature_ctx, (uint8_t*)temp_buffer, block_size);
        }
    ei_printf("Sampling finished\n");
    //free(sampleBuffer);
    return true;
}

bool ei_microphone_setup_data_sampling(void)
{
    // this sensor does not have settable interval...
    // ei_config_set_sample_interval(static_cast<float>(1000) / static_cast<float>(AUDIO_SAMPLING_FREQUENCY));
    int sample_length_blocks;

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
    ei_printf("block_size: %d \n", block_size);
    // sampleBuffer = (int16_t *)pvPortMalloc(ei_sfud_fs_get_block_size());

    // if (sampleBuffer == NULL) {
    //     return false;
    // }

    bool r = ei_microphone_record(ei_config_get_config()->sample_length_ms, (((samples_required <<1)/ ei_sfud_fs_get_block_size()) * SFUD_FS_BLOCK_ERASE_TIME_MS), samples_required);

    //vPortFree(sampleBuffer);
    delay(100);

    uint8_t final_byte[] = { 0xff };
    int ctx_err = ei_mic_ctx.signature_ctx->update(ei_mic_ctx.signature_ctx, final_byte, 1);
    if (ctx_err != 0) {
        return ctx_err;
    }

    ei_printf("Finalizing signature\n");
    ctx_err = ei_mic_ctx.signature_ctx->finish(ei_mic_ctx.signature_ctx, ei_mic_ctx.hash_buffer.buffer);
    if (ctx_err != 0) {
        ei_printf("Failed to finish signature (%d)\n", ctx_err);
        return false;
    }

    ei_printf("allocate a page buffer");
    // static uint8_t page_buffer[4096];
    // if (!page_buffer) {
    //     ei_printf("Failed to allocate a page buffer to write the hash\n");
    //     return false;
    // }   
    //ei_printf("Finish\n");
    ei_printf("read first page");
    int j = ei_sfud_fs_read_sample_data(temp_buffer, 0, ei_sfud_fs_get_block_size());
    if (j != 0) {
        ei_printf("Failed to read first page (%d)\n", j);
        //vPortFree(page_buffer);
        return false;
    }

    ei_printf("update the hash");
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
    ei_printf("erase first page");
    j = ei_sfud_fs_erase_sampledata(0, ei_sfud_fs_get_block_size());
    if (j != 0) {
        ei_printf("Failed to erase first page (%d)\n", j);
        //vPortFree(page_buffer);
        return false;
    }
    ei_printf("write_sample");
    j = ei_sfud_fs_write_samples(temp_buffer, 0, ei_sfud_fs_get_block_size());

    //vPortFree(page_buffer);

    if (j != 0) {
        ei_printf("Failed to write first page with updated hash (%d)\n", j);
        return false;
    }

    finish_and_upload(filename, ei_config_get_config()->sample_length_ms);

    return true;
}