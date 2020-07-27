#include "setup.h"
#include "ingestion-sdk-platform/wio-terminal/ei_device_wio_terminal.h"
#include "repl/at_cmd_repl_freertos.h"
#include "sfud_fs_commands.h"
#if CM_DEBUG
#include "cm_backtrace.h"
#else
//fake cm_backtrace_fault
extern "C"
{
    void cm_backtrace_fault(uint32_t fault_handler_lr, uint32_t fault_handler_sp)
    {
        return;
    }
}
#endif

#define HARDWARE_VERSION "seeed_wio_terminal"
#define SOFTWARE_VERSION "V0.1.0"

static AtCmdRepl repl(ei_get_serial());

void print_memory_info()
{
    //
}

static bool ei_sfud_fs_read_buffer(size_t begin, size_t length, void (*data_fn)(uint8_t *, size_t))
{

    size_t pos = begin;
    size_t bytes_left = length;
    // we're encoding as base64 in AT+READFILE, so this needs to be divisable by 3
    uint8_t buffer[513];
    while (1)
    {
        size_t bytes_to_read = sizeof(buffer);
        if (bytes_to_read > bytes_left)
        {
            bytes_to_read = bytes_left;
        }
        if (bytes_to_read == 0)
        {
            return true;
        }

        int r = ei_sfud_fs_read_sample_data(buffer, pos, bytes_to_read);
        if (r != 0)
        {
            return false;
        }
        data_fn(buffer, bytes_to_read);

        pos += bytes_to_read;
        bytes_left -= bytes_to_read;
    }

    return true;
}
static ei_config_ctx_t config_ctx = {0};

void ei_main()
{
    ei_printf("Edge Impulse standalone inferencing (FreeRTOS)\n");
#if CM_DEBUG
    cm_backtrace_init("edge-impulse.ino.Seeeduino.samd", HARDWARE_VERSION, SOFTWARE_VERSION);
#endif
    ei_sfud_fs_init();

    // ei_config_ctx_t config_ctx = { 0 }; //会出现段错误，why ?
    config_ctx.get_device_id = EiDevice.get_id_function();
    config_ctx.get_device_type = EiDevice.get_type_function();
    config_ctx.list_files = NULL;

    config_ctx.load_config = &ei_sfud_fs_load_config;
    config_ctx.save_config = &ei_sfud_fs_save_config;
    config_ctx.list_files = NULL;
    config_ctx.read_buffer = &ei_sfud_fs_read_buffer;

    EI_CONFIG_ERROR cr = ei_config_init(&config_ctx);

    if (cr != EI_CONFIG_OK)
    {
        ei_printf("Failed to initialize configuration (%d)\n", cr);
    }
    else
    {
        ei_printf("Loaded configuration\n");
    }

    // ei_sfud_fs_write_samples()
    // ei_sfud_fs_read_sample_data

    ei_at_register_generic_cmds();
    repl.start_repl();

    Thread::StartScheduler();

    while (1)
    {
    }
}
