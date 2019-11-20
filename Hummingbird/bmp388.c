/*
 * bmp388.c
 *
 * Created: 9/16/2019 9:25:01 PM
 *  Author: Wesley
 */

#include "atmel_start.h"
#include "atmel_start_pins.h"
#include "error.h"
#include <stdbool.h>
#include <stdint.h>

static uint8_t bmp388_read_register(uint8_t address);
static void bmp388_read_registers(uint8_t address, uint8_t *data, uint8_t length);
static void bmp388_write_register(uint8_t address, uint8_t value);
static void load_calibration();

typedef enum bmp388_mode_t { SLEEP, FORCED, NORMAL } bmp388_mode_t;
static void enable_and_set_mode(bool pressure, bool temperature,
                                bmp388_mode_t mode);

static const uint8_t READ_MASK = 0x80; // Set bit 7 high for read

static const uint8_t BMP388_REG_CHIP_ID = 0x00;
static const uint8_t BMP388_REG_STATUS = 0x03;
static const uint8_t BMP388_REG_DATA = 0x04;
static const uint8_t BMP388_REG_PWR_CTRL = 0x1B;
static const uint8_t BMP388_REG_CALIBRATION = 0x31;
static const uint8_t BMP388_REG_CMD = 0x7E;

static const uint8_t BMP388_CMD_RESET = 0xB6;

static struct io_descriptor *io;


typedef struct calibration_data {
	double par_t1;
	double par_t2;
	double par_t3;
	
	double par_p1;
	double par_p2;
	double par_p3;
	double par_p4;
	double par_p5;
	double par_p6;
	double par_p7;
	double par_p8;
	double par_p9;
	double par_p10;
	double par_p11;
	
	double t_lin;
} calibration_data;

static calibration_data calibration_datas = { 0 };


void bmp388_reset(void) {
	bmp388_write_register(BMP388_REG_CMD, BMP388_CMD_RESET);
}

void bmp388_get_reading(void) {
	enable_and_set_mode(false, false, SLEEP);
	delay_ms(5);
	enable_and_set_mode(true, true, FORCED);
	while(true) {
		uint8_t status = bmp388_read_register(BMP388_REG_STATUS);
		if(status & 0x60) { // check bits 6 and 7 are set
			break;
		}
	}
	uint8_t data[6] = { 0 };
	bmp388_read_registers(BMP388_REG_DATA, data, sizeof(data));
	
	uint32_t lsb = (uint32_t) data[0];
	uint32_t asb = (uint32_t) data[1] << 8;
	uint32_t msb = (uint32_t) data[2] << 16;
	
	uint32_t pressure = msb | asb | lsb;
	
	lsb = (uint32_t) data[3];
	asb = (uint32_t) data[4] << 8;
	msb = (uint32_t) data[5] << 16;
	
	uint32_t raw_temperature = msb | asb | lsb;
	
	double partial_data1 = ((double) raw_temperature) - calibration_datas.par_t1;
	double partial_data2 = partial_data1 * calibration_datas.par_t2;
	
	volatile double final = partial_data2 + (partial_data1 * partial_data1) * calibration_datas.par_t3;
	
	  __asm__("BKPT");
}

void bmp388_init(void) {
  spi_m_sync_get_io_descriptor(&SPI_2, &io);
  spi_m_sync_enable(&SPI_2);

  bmp388_reset();

  volatile uint8_t result = bmp388_read_register(BMP388_REG_CHIP_ID);
  if (result != 0x50) {
    error(BMP388_INIT_FAIL);
  }
  
  load_calibration();
  
  while(true) {
	bmp388_get_reading();
  }
}

static void load_calibration() {
	uint8_t calibration[21] = { 0 };
	
	bmp388_read_registers(BMP388_REG_CALIBRATION, calibration, sizeof(calibration));
	
	uint16_t nvm_par_t1 = (calibration[1] << 8) | calibration[0];
	calibration_datas.par_t1 = ((double) nvm_par_t1) / 0.00390625d; // 1 / 2^ 8
	uint16_t nvm_par_t2 = (calibration[3] << 8) | calibration[2];
	calibration_datas.par_t2 = ((double)nvm_par_t2) / 1073741824.0d; // 2 ^ 30
	uint8_t nvm_par_t3 = calibration[4];
	calibration_datas.par_t3 = ((double) nvm_par_t3) / 281474976710656.0d; // 2^ 48
	
	uint16_t nvm_par_p1 = (calibration[6] << 8) | calibration[5];
	uint16_t nvm_par_p2 = (calibration[8] << 8) | calibration[7];
	uint8_t nvm_par_p3 = calibration[9];
	uint8_t nvm_par_p4 = calibration[10];
	uint16_t nvm_par_p5 = (calibration[12] << 8) | calibration[11];
	uint16_t nvm_par_p6 = (calibration[14] << 8) | calibration[13];
	uint8_t nvm_par_p7 = calibration[15];
	uint8_t nvm_par_p8 = calibration[16];
	uint16_t nvm_par_p9 = (calibration[18] << 8) | calibration[17];
	uint8_t nvm_par_p10 = calibration[19];
	uint8_t nvm_par_p11 = calibration[20];
}


static void enable_and_set_mode(bool pressure, bool temperature,
                                bmp388_mode_t mode) {
  uint8_t value = 0;
  value = bmp388_read_register(BMP388_REG_PWR_CTRL);
  if (pressure) {
    value |= (1 << 0); // set bit 0
  } else {
	  //clear bit
  }
  if (temperature) {
    value |= (1 << 1); // set bit 1
  } else {
	  //clear bit
  }

  switch (mode) { // set bits 4 and 5
  case SLEEP:
    // nothing, is zero
    break;
  case FORCED:
    value |= (1 << 4); // 01
    break;
  case NORMAL:
    value |= (2 << 4); // 11
    break;
  }
  bmp388_write_register(BMP388_REG_PWR_CTRL, value);
}

static uint8_t bmp388_read_register(uint8_t address) {
  address = address | READ_MASK;
  uint8_t address_and_dummy_byte[2];
  address_and_dummy_byte[0] = address;
  address_and_dummy_byte[1] = 0;

  uint8_t result;
  gpio_set_pin_level(BMP388_CS, false);
  io_write(io, address_and_dummy_byte, sizeof(address_and_dummy_byte));
  io_read(io, &result, 1);
  gpio_set_pin_level(BMP388_CS, true);
  return result;
}

static void bmp388_read_registers(uint8_t address, uint8_t *data, uint8_t length) {
	address = address | READ_MASK;
  uint8_t address_and_dummy_byte[2];
  address_and_dummy_byte[0] = address;
  address_and_dummy_byte[1] = 0;

  gpio_set_pin_level(BMP388_CS, false);
  io_write(io, address_and_dummy_byte, sizeof(address_and_dummy_byte));
  io_read(io, data, length);
  gpio_set_pin_level(BMP388_CS, true);
}

static void bmp388_write_register(uint8_t address, uint8_t value) {
  gpio_set_pin_level(BMP388_CS, false);
  uint8_t data[2] = {address, value};
  io_write(io, data, sizeof(data));
  gpio_set_pin_level(BMP388_CS, true);
}
