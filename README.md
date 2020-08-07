# Edge Impulse firmware for Seeed Wio Terminal
Edge Impulse enables developers to create the next generation of intelligent device solutions with embedded Machine Learning. This repository contains the Edge Impulse firmware for the Seeed Samd Serial development board. This device supports all Edge Impulse device features, including ingestion, remote management and inferencing.
## Introduction

## Requirements
### Hardware
- [Seeed Wio Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html)
### Tools 
The arduino-cli tool is used to build and upload the Edge Impulse firmware to the Seeed Wio terminal board. Use following link for download and installation procedure:
* [Arduino CLI](https://arduino.github.io/arduino-cli/installation/).


## Building the application
The Edge Impulse firmware depends on libraries and the samd core for Arduino. Running the following script will install all the dependencies for you:

```shell

cd ~/Arduino/libraries/
git clone https://github.com/Seeed-Studio/Seeed_Arduino_FreeRTOS
git clone https://github.com/Seeed-Studio/Seeed_Arduino_ooFreeRTOS
git clone https://github.com/Seeed-Studio/Seeed_Arduino_LIS3DHTR
git clone https://github.com/Seeed-Studio/Seeed_Arduino_mbedtls -b edge-impulse
git clone https://github.com/Seeed-Studio/Seeed_Arduino_MultiGas
git clone https://github.com/Seeed-Studio/Seeed_Arduino_SFUD
git clone https://github.com/Seeed-Studio/Grove_BME280
git clone https://github.com/Seeed-Studio/Seeed_Arduino_DPS310
git clone https://github.com/Seeed-Studio/Seeed_SCD30
git clone https://github.com/Seeed-Studio/Grove_6Axis_Accelerometer_And_Gyroscope_BMI088
git clone https://github.com/Seeed-Studio/Seeed_Arduino_UltrasonicRanger
git clone https://github.com/Seeed-Studio/Seeed_Arduino_TFlidar
git clone https://github.com/Seeed-Studio/Seeed_Arduino_edgeimpulse
cd  Seeed_Arduino_edgeimpulse 
./arduino-build.sh --build
```
