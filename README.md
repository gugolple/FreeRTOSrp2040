# RP2040 FreeRTOS

This is a basic project to test and learn FreeRTOS executed on an RP2040 due to it's simplicity and ubiquity.

## Goals

The following tasks are intended to be realized:

- Track the time over an LCD (HD44780 - 8 wire configuration)
- Keep the time (RTC type of operation/addon)
- RFID/NFC access and display over the screen

### Restrictions

The idea of the operation is to learn the microcontroller and not to produce a quick solution, for that the project will be realize only supported by standard libraries (like C11/C++11 and boost) and what is provided by FreeRTOS.

# References

Based on [FreeRTOS LTS](https://github.com/FreeRTOS/FreeRTOS-LTS) release [202406.1TLS](https://github.com/FreeRTOS/FreeRTOS-LTS/releases/tag/202406.01-LTS) path: "FreeRTOS/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040"
