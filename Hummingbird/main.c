/**
 * \file
 *
 * \brief Application implement
 *
 * Copyright (c) 2015-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip
 * Support</a>
 */

#include "atmel_start.h"
#include "atmel_start_pins.h"
#include "bmp388.h"
#include "crc.h"
#include "rfm9x.h"
#include "spi_flash.h"
#include <stdio.h>

float read_voltage();

const bool USB_ENABLED = false;

typedef struct Datapoint {
  double temperature; // 8 bytes
  double pressure; //8 bytes 
  float battery_voltage; // 4 bytes
  uint32_t packet_number; // 4 bytes
  uint32_t flight_number; // 4 bytes
  uint8_t device_id; // 1 bytes
  uint8_t version; // 1 bytes
  crc_t crc8; // 1 bytes
} Datapoint;

const uint8_t VERSION = 1;
const uint8_t DEVICE_ID	= 1;
const uint8_t DATAPOINT_TO_CRC = 30;

int main(void) {
  atmel_start_init();

  if (USB_ENABLED) {
    wait_for_cdc_ready();
  }

  adc_sync_enable_channel(&ADC_0, 0);

  rfm9x_init();
  bmp388_init();
  spi_flash_init();

  bmp_reading reading = {0};
  Datapoint datapoint = {0};
  datapoint.device_id = DEVICE_ID;
  datapoint.version	= VERSION;
  uint32_t packet_number = 0;

  while (1) {
    delay_ms(5000);
    gpio_toggle_pin_level(LED2);
    bmp388_get_reading(&reading);

    datapoint.battery_voltage = read_voltage();
    datapoint.temperature = reading.temperature;
    datapoint.pressure = reading.pressure;
    datapoint.packet_number = packet_number;
    datapoint.flight_number = 42;

	uint8_t* raw = (uint8_t*) &datapoint;
	
	volatile crc_t crc = crc_init();
	crc = crc_update(crc, raw, DATAPOINT_TO_CRC);
	crc = crc_finalize(crc);
	datapoint.crc8 = crc;
	rfm9x_send(raw, sizeof(Datapoint));
    packet_number++;
    // __asm__("BKPT");
  }
}


float read_voltage() {
  uint16_t raw_battery_voltage;

  adc_sync_read_channel(&ADC_0, 0, ((uint8_t *)&raw_battery_voltage), 2);
  adc_sync_read_channel(&ADC_0, 0, ((uint8_t *)&raw_battery_voltage), 2);

  float battery_voltage = (float)raw_battery_voltage;
  battery_voltage *= 2;    // we divided by 2, so multiply back
  battery_voltage *= 3.3;  // Multiply by 3.3V, our reference voltage
  battery_voltage /= 4096; // convert to voltage
  return battery_voltage;

  /*
  if (USB_ENABLED) {
    cdcdf_acm_write((uint8_t *)buffer, strlen(buffer));
  }
  */
}

