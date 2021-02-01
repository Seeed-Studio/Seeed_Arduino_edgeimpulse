# Edge Impulse firmware for Seeed Wio Terminal
Edge Impulse enables developers to create the next generation of intelligent device solutions with embedded Machine Learning. This repository contains the Edge Impulse firmware for the Seeed Samd Serial development board. This device supports all Edge Impulse device features, including ingestion, remote management and inferencing.
## Introduction

## Requirements
### Hardware
- [Seeed Wio Terminal](https://www.seeedstudio.com/Wio-Terminal-p-4509.html)
### Tools 
The arduino-cli tool is used to build and upload the Edge Impulse firmware to the Seeed Wio terminal board. Use following link for download and installation procedure:
* [Arduino CLI](https://arduino.github.io/arduino-cli/installation/).


## Building and flashing the firmware

```shell

git submodule update --init --recursive -j 8
./arduino-cli config set directories.user ./user #that will set your user direcotry for Arduini CLI to user folder in current directory - change back after you finish building 
./arduino-build.sh --build
arduino-cli upload --fqbn=Seeeduino:samd:seeed_wio_terminal -p /dev/ttyACM0 -i build/*.bin
```
