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
#include <stdio.h>

void read_voltage();
void test_spi_flash();
void test_rfm9x();

const bool USB_ENABLED = false;

int main(void) {
  atmel_start_init();

  if (USB_ENABLED) {
    wait_for_cdc_ready();
  }

  adc_sync_enable_channel(&ADC_0, 0);
  while (1) {
    delay_ms(1000);
    gpio_toggle_pin_level(LED2);
    read_voltage();
    //   test_spi_flash();
    test_rfm9x();
  }
}

const uint8_t RFM95_REG_OP_MODE[] = {0x01};
const uint8_t RFM95_REG_VERSION[] = {0x42};
const uint8_t OP_MODE_SLEEP = 0x0;
const uint8_t OP_MODE_LORA = 0x80;

void test_rfm9x() {
  struct io_descriptor *io;
  spi_m_sync_get_io_descriptor(&SPI_1, &io);
  spi_m_sync_enable(&SPI_1);
  
  /*
  gpio_set_pin_level(LORA_CS, false);
  io_write(io, RFM95_REG_VERSION, sizeof(RFM95_REG_VERSION));
  uint8_t version;
  io_read(io, &version, 1);
  gpio_set_pin_level(LORA_CS, true);
  this is returning 12, which is weird, the datasheet says it should be 11? hoping new version...
  */

  gpio_set_pin_level(LORA_CS, false);
  io_write(io, RFM95_REG_OP_MODE, sizeof(RFM95_REG_OP_MODE));
  uint8_t mode[] = {OP_MODE_SLEEP};
  io_write(io, mode, 1);
  gpio_set_pin_level(LORA_CS, true);
  delay_ms(10);
  
  gpio_set_pin_level(LORA_CS, false);
  io_write(io, RFM95_REG_OP_MODE, sizeof(RFM95_REG_OP_MODE));
  uint8_t actual_mode;
  io_read(io, &actual_mode, 1);
  gpio_set_pin_level(LORA_CS, true);
  //not going into sleep mode...
}

const uint8_t W25_CMD_POWER_ON[] = {
    0xAB,
    0x00,
    0x00,
    0x00,
};
const uint8_t W25_CMD_READ_MANUFACTER_ID[] = {0x90, 0x00, 0x00};
const uint8_t W25_CMD_READ_JEDEC_ID[] = {0x9F};

void test_spi_flash() {
  struct io_descriptor *io;
  spi_m_sync_get_io_descriptor(&SPI_0, &io);

  spi_m_sync_enable(&SPI_0);

  gpio_set_pin_level(FLASH_CS, false);
  io_write(io, W25_CMD_POWER_ON, sizeof(W25_CMD_POWER_ON));
  uint8_t result;
  io_read(io, &result, 1);
  gpio_set_pin_level(FLASH_CS, true);

  gpio_set_pin_level(FLASH_CS, false);
  io_write(io, W25_CMD_READ_MANUFACTER_ID, sizeof(W25_CMD_READ_MANUFACTER_ID));
  uint8_t manufacuter_id[3];
  io_read(io, manufacuter_id, sizeof(manufacuter_id));
  gpio_set_pin_level(FLASH_CS, true);

  gpio_set_pin_level(FLASH_CS, false);
  io_write(io, W25_CMD_READ_JEDEC_ID, sizeof(W25_CMD_READ_JEDEC_ID));
  uint8_t jedec_id[3];
  io_read(io, jedec_id, sizeof(jedec_id));
  gpio_set_pin_level(FLASH_CS, true);
}

void read_voltage() {
  uint16_t raw_battery_voltage;

  adc_sync_read_channel(&ADC_0, 0, ((uint8_t *)&raw_battery_voltage), 2);
  adc_sync_read_channel(&ADC_0, 0, ((uint8_t *)&raw_battery_voltage), 2);

  float battery_voltage = (float)raw_battery_voltage;
  battery_voltage *= 2;    // we divided by 2, so multiply back
  battery_voltage *= 3.3;  // Multiply by 3.3V, our reference voltage
  battery_voltage /= 4096; // convert to voltage
  char buffer[120];
  sprintf(buffer, "ADC value is %d converted to %.6f also %.6f\n",
          raw_battery_voltage, battery_voltage, 3.3F);
  if (USB_ENABLED) {
    cdcdf_acm_write((uint8_t *)buffer, strlen(buffer));
  }
}

