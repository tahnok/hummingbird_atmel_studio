/*
 * error.c
 *
 * Created: 9/16/2019 8:38:44 PM
 *  Author: Wesley
 */

#include "error.h"
#include "atmel_start.h"
#include "atmel_start_pins.h"

void error(__attribute__ ((unused)) ERROR_REASON reason) {
  __asm__("BKPT");
  while (1) {
    delay_ms(100);
    gpio_toggle_pin_level(LED2);
  }
}
