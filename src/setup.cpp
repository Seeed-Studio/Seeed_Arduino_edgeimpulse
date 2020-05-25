#include "setup.h"
#include "wio-terminal/ei_device_wio_terminal.h"
#include "repl/at_cmd_repl_freertos.h"
static AtCmdRepl repl(ei_get_serial());


void print_memory_info() {
   //
}


void ei_main() {
    ei_printf("Edge Impulse standalone inferencing (FreeRTOS)\n");

    ei_at_register_generic_cmds();

    repl.start_repl();

    Thread::StartScheduler();

    while (1) {
        
    }

}
