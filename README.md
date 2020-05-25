# Seeed Arduino edgeimpulse 

## Introduction

## Requirements
### Hardware
- Seeeduino Wio Terminal
### Tools 
- Arduino CLI
- Arduino IDE

### Boards Manager URL
```shell
https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
```
###  Dependency Library
- [Seeed_Arduino_FreeRTOS](https://github.com/Seeed-Studio/Seeed_Arduino_FreeRTOS)
- [Seeed_Arduino_ooFreeRTOS](https://github.com/Seeed-Studio/Seeed_Arduino_ooFreeRTOS)

## Building the application
- Download project
```shell
$ git clone https://github.com/Seeed-Studio/Seeed_Arduino_edgeimpulse edge-impulse
```
- Arduino CLI 
```shell
$ arduino-cli compile  --warnings all --fqbn Seeeduino:samd:seeed_wio_terminal --verbose  edge-impulse.ino 
```
- Arduino IDE
    - open edge-implus.ino
    - compile & upload

## AT Reference


----
## License

