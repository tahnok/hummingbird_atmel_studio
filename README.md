# Hummingbird Firmware

Source code for Hummingbird kite telemetry using [Atmel Studio](https://www.microchip.com/mplab/avr-support/atmel-studio-7) and [ASF4](https://www.microchip.com/mplab/avr-support/advanced-software-framework)

## System description

An Atmel ARM [ATSAMD21G18](https://www.microchip.com/wwwproducts/en/ATsamd21g18) is connected over SPI to the following periphreals

 - HopeRF RFM95 915MHz LoRa Radio

    Status: Responds to SPI packets, haven’t transmitted or received anything yet

 - Bosch BMP388 Barometric Pressure Sensor

    Status: Responds over SPI, working on driver

 - Winbond W25 SPI Flash (64 MB)

    Status: works

 - MicroSD card holder

    Status: unusable since it’s mounted backwards
    
    
    ## Programming
    
    Programing and testing is done using a JLink Edu Mini
