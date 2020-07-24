
#include "Arduino.h"
#include "Wire.h"

bool i2c_scanner(TwoWire &wire, uint8_t address)
{
    uint8_t error = 0;

    wire.beginTransmission(address);

    error = Wire.endTransmission();

    if (error == 0)
    {
        wire.beginTransmission(0x1);
        error = Wire.endTransmission();
        if (error == 0)
        {
            wire.beginTransmission(0x7F);
            error = Wire.endTransmission();
            if(error == 0)
                return false;
        }
    }

   return true;
}