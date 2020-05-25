
#include "Seeed_Arduino_FreeRTOS.h"
#include "thread.hpp"

using namespace cpp_freertos;
#include "repl/at_cmd_repl_FreeRTOS.h"

    // void theard(void* pvParameters)
    // {
    //     while(1)
    //     {
    //        // callback_irq();
    //         vTaskDelay(10);
    //     }
    //     vTaskDelete(NULL);
    // }

void setup()
{
    
    Serial.begin(115200);
    while (!Serial)
    {
        /* code */
    }
    ei_at_register_generic_cmds();
    static AtCmdRepl repl(&Serial);
    repl.start_repl();

    Thread::StartScheduler();
}

void loop()
{
    
}