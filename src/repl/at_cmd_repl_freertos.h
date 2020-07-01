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

#ifndef _EDGE_IMPULSE_AT_COMMANDS_REPL_MBED_H_
#define _EDGE_IMPULSE_AT_COMMANDS_REPL_MBED_H_

#define EI_SIGNAL_TERMINATE_THREAD_REQ 0x1
#define EI_SIGNAL_TERMINATE_THREAD_RES 0x4

/**
 * Mbed OS specific code for handling the REPL and throwing AT
 * commands around...
 */
#include "at_cmds.h"
#include "repl.h"
#include "thread.hpp"
#include "ticks.hpp"
#include "message.hpp"
#include "semaphore.hpp"
#include "Arduino.h"
using namespace cpp_freertos;

typedef struct
{
    char cmd[256];
} ei_at_mail_t;

class AtCmdRepl : public Thread
{
public:
    AtCmdRepl(SerialStream *serial)
        : Thread(8 * 1024, 2),
          _repl(serial),
          Mail(256),
          _mail_handled(1),
          _terminate_thread(false)
    {
    }

    /**
     * Start the REPL
     * (note that you'll need to run the event queue as well!)
     */
    void start_repl()
    {
        std::function<bool(const char *)> callback = std::bind(&AtCmdRepl::exec_command, this, placeholders::_1);
        _repl.start(callback);
        Start();
    }

    /**
     * When executing a command we'll detach the UART interrupts from the REPL
     * and handle them ourselves, to allow breaking off a command...
     */
    bool exec_command(const char *cmd)
    {

        if (strlen(cmd) > 255)
        {
            ei_printf("Command is too long (max. length 255)\n");
            return true;
        }

        _terminate_thread = false;
        // wait for the semaphore to fire
        _mail_handled.Take();
        _repl.stop();
        Mail.Send((void *)cmd, strlen(cmd));


        // event was cancelled
        // if (_terminate_thread)
        // {
        //     ei_printf("I am here2\n\r");
        //     for (size_t ix = 0; ix < 10; ix++)
        //     {
        //         Delay(Ticks::MsToTicks(200));
        //         if (eTaskGetState(this->GetHandle()) != eReady)
        //         {
        //             break;
        //         }
        //     }

        //     //this->Cleanup();
        //     // vTaskDelete(this->GetHandle());

        //     //this->Start();
        // }
        _terminate_thread = false;
        return false;
    }

    void break_inference_loop_irq()
    {
        if (_repl.getSerial()->available() && _repl.getSerial()->read() == 'b')
        {
            _terminate_thread = true;
            // release the semaphore so the main thread knows the event is done
            _mail_handled.Give();
        }
    }

    void reprintReplState()
    {
        _repl.reprintReplState();
    }

private:
    /**
     * Main thread for commands, pretty straight forward
     * it will wait for an item in the mailbox, execute it, then use the semaphore to signal back that we're done
     */
    void Run()
    {
        while (1)
        {
            size_t nums = Mail.Receive(&buff, 256);
            if (nums > 0)
            {
                //ei_at_cmd_handle(buff);
                // Serial.printf("Receive: ");
                // for (int i = 0; i < nums; i++)
                // {
                //     Serial.write(buff[i]);
                // }
                // Serial.printf(", length: %d\n\r", nums);
                // signal to other thread

                ei_at_cmd_handle((const char *)&buff, nums);

                // and command is handled, restart REPL
                std::function<bool(const char *)> callback = std::bind(&AtCmdRepl::exec_command, this, placeholders::_1);
                _repl.start(callback);

                if (!_terminate_thread)
                {
                    // so this shouldn't be an issue I'd say but release'ing this again
                    // (whilst it was also released in break_inference_loop_irq)
                    // seems to break the state (and we can no longer aquire this semaphore later)
                    _mail_handled.Give();
                    //Serial.printf("Unlock\n\r");
                }
            }
        }
    }
    Repl _repl;
    Message Mail;
    BinarySemaphore _mail_handled;
    uint8_t buff[256];
    bool _terminate_thread;
};

#endif // _EDGE_IMPULSE_AT_COMMANDS_REPL_MBED_H_
