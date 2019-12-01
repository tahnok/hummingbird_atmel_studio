/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */
#ifndef ATMEL_START_PINS_H_INCLUDED
#define ATMEL_START_PINS_H_INCLUDED

#include <hal_gpio.h>

// SAMD21 has 8 pin functions

#define GPIO_PIN_FUNCTION_A 0
#define GPIO_PIN_FUNCTION_B 1
#define GPIO_PIN_FUNCTION_C 2
#define GPIO_PIN_FUNCTION_D 3
#define GPIO_PIN_FUNCTION_E 4
#define GPIO_PIN_FUNCTION_F 5
#define GPIO_PIN_FUNCTION_G 6
#define GPIO_PIN_FUNCTION_H 7

#define BATT_V GPIO(GPIO_PORTA, 4)
#define LORA_RESET GPIO(GPIO_PORTA, 6)
#define LORA_INT GPIO(GPIO_PORTA, 7)
#define BMP388_MOSI GPIO(GPIO_PORTA, 12)
#define BMP388_SCK GPIO(GPIO_PORTA, 13)
#define BMP388_MISO GPIO(GPIO_PORTA, 14)
#define BMP388_CS GPIO(GPIO_PORTA, 15)
#define FLASH_MOSI GPIO(GPIO_PORTA, 16)
#define FLASH_SCK GPIO(GPIO_PORTA, 17)
#define FLASH_MISO GPIO(GPIO_PORTA, 18)
#define FLASH_CS GPIO(GPIO_PORTA, 19)
#define USB_DM GPIO(GPIO_PORTA, 24)
#define USB_DP GPIO(GPIO_PORTA, 25)
#define LED2 GPIO(GPIO_PORTA, 27)
#define LORA_MOSI GPIO(GPIO_PORTB, 8)
#define LORA_SCK GPIO(GPIO_PORTB, 9)
#define LORA_MISO GPIO(GPIO_PORTB, 10)
#define LORA_CS GPIO(GPIO_PORTB, 11)

#endif // ATMEL_START_PINS_H_INCLUDED
