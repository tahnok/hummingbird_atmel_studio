/*
 * bmp388.c
 *
 * Created: 9/16/2019 9:25:01 PM
 *  Author: Wesley
 */

#include "atmel_start.h"
#include "atmel_start_pins.h"
#include "error.h"
#include "bmp388.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

static uint8_t bmp388_read_register(uint8_t address);
static void bmp388_read_registers(uint8_t address, uint8_t *data,
                                  uint8_t length);
static void bmp388_write_register(uint8_t address, uint8_t value);
static void load_calibration();

typedef enum bmp388_mode_t { SLEEP, FORCED, NORMAL } bmp388_mode_t;
static void enable_and_set_mode(bool pressure, bool temperature,
                                bmp388_mode_t mode);

static double parse_temperature(uint8_t data_3, uint8_t data_4, uint8_t data_5);
static double parse_pressure(uint8_t data_0, uint8_t data_1, uint8_t data_2);

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

static calibration_data calibration = {0};

void bmp388_init(void) {
  spi_m_sync_get_io_descriptor(&SPI_2, &io);
  spi_m_sync_enable(&SPI_2);

  bmp388_reset();

  uint8_t result = bmp388_read_register(BMP388_REG_CHIP_ID);
  if (result != 0x50) {
    error(BMP388_INIT_FAIL);
  }

  load_calibration();
}

void bmp388_reset(void) {
  bmp388_write_register(BMP388_REG_CMD, BMP388_CMD_RESET);
}

void bmp388_get_reading(bmp_reading* reading) {
  enable_and_set_mode(false, false, SLEEP);
  delay_ms(5);
  enable_and_set_mode(true, true, FORCED);
  while (true) {
    uint8_t status = bmp388_read_register(BMP388_REG_STATUS);
    if (status & 0x60) { // check bits 6 and 7 are set
      break;
    }
  }
  uint8_t raw_reading[6] = {0};
  bmp388_read_registers(BMP388_REG_DATA, raw_reading, sizeof(raw_reading));

  volatile double temperature =
      parse_temperature(raw_reading[3], raw_reading[4], raw_reading[5]);
  volatile double pressure =
      parse_pressure(raw_reading[0], raw_reading[1], raw_reading[2]);

  reading->temperature = temperature;
  reading->pressure = pressure;
}

/*
Take the data from the 3 temperature registers and converts them, corrects them
and returns the temperature

ALSO, set calibration.t_lin, since temperature is needed for pressure

See Section 9.2 in the BMP388 Datasheet
*/
static double parse_temperature(uint8_t data_3, uint8_t data_4, uint8_t data_5) {
  uint32_t xlsb = (uint32_t)data_3;
  uint32_t lsb = (uint32_t)data_4 << 8;
  uint32_t msb = (uint32_t)data_5 << 16;

  uint32_t raw_temperature = msb | lsb | xlsb;

  double partial_data1 = ((double)raw_temperature) - calibration.par_t1;
  double partial_data2 = partial_data1 * calibration.par_t2;

  volatile double temperature =
      partial_data2 + (partial_data1 * partial_data1) * calibration.par_t3;

  calibration.t_lin = temperature;

  return temperature;
}

static double parse_pressure(uint8_t data_0, uint8_t data_1, uint8_t data_2) {
  uint32_t xlsb = (uint32_t)data_0;
  uint32_t lsb = (uint32_t)data_1 << 8;
  uint32_t msb = (uint32_t)data_2 << 16;

  uint32_t raw_pressure = msb | lsb | xlsb;
  
  double partial_data1;
  double partial_data2;
  double partial_data3;
  double partial_data4;
  
  partial_data1 = calibration.par_p6 * calibration.t_lin;
  partial_data2 = calibration.par_p7 * pow(calibration.t_lin, 2.0d);
  partial_data3 = calibration.par_p8 * pow(calibration.t_lin, 3.0d);
  
  double partial_out1 = calibration.par_p5 + partial_data1 + partial_data2 + partial_data3;
  
  partial_data1 = calibration.par_p2 * calibration.t_lin;
  partial_data2 = calibration.par_p3 * pow(calibration.t_lin, 2.0d);
  partial_data3 = calibration.par_p4 * pow(calibration.t_lin, 3.0d);
  
  double partial_out2 = ((double) raw_pressure) * (calibration.par_p1 + partial_data1 + partial_data2 + partial_data3);
  
  partial_data1 = pow(((double) raw_pressure), 2.0d);
  partial_data2 = calibration.par_p9 + calibration.par_p10 * calibration.t_lin;
  partial_data3 = partial_data1 * partial_data2;
  partial_data4 = partial_data3 + pow(((double) raw_pressure), 3.0d) * calibration.par_p11;
  
  return partial_out1 + partial_out2 + partial_data4;
}

inline static uint16_t byte_concat(uint8_t msb, uint8_t lsb) {
  return (((uint16_t) msb) << 8) | ((uint16_t) lsb);
}

/*
Read calibration registers and calculate calibration parameters

See BMP388 datasheet, section 9.1 for formulas, section 3.11.1 for register
layouts
*/
static void load_calibration() {
  uint8_t raw_calibration[21] = {0};

  bmp388_read_registers(BMP388_REG_CALIBRATION, raw_calibration,
                        sizeof(raw_calibration));

  uint16_t nvm_par_t1 = byte_concat(raw_calibration[1], raw_calibration[0]);
  calibration.par_t1 = ((double)nvm_par_t1) / 0.00390625d; // 1 / 2^-8

  uint16_t nvm_par_t2 = byte_concat(raw_calibration[3], raw_calibration[2]);
  calibration.par_t2 = ((double)nvm_par_t2) / 1073741824.0d; // 2 ^ 30

  int8_t nvm_par_t3 = (int8_t) raw_calibration[4];
  calibration.par_t3 = ((double)nvm_par_t3) / 281474976710656.0d; // 2^ 48


  int16_t nvm_par_p1 = (int16_t) byte_concat(raw_calibration[6], raw_calibration[5]);
  calibration.par_p1 =
      ((double)(nvm_par_p1 - 16384)) / 1048576.0d; // (nvm_par_p1 - 2^14) / 2^20

  int16_t nvm_par_p2 = (int16_t) byte_concat(raw_calibration[8], raw_calibration[7]);
  calibration.par_p2 = ((double)(nvm_par_p2 - 16384)) /
                       536870912.0d; // (nvm_par_p2 - 2^14) / 2^29

  int8_t nvm_par_p3 = (int8_t) raw_calibration[9];
  calibration.par_p3 =
      ((double)nvm_par_p3) / 4294967296.0d; // nvm_par_p3 / 2^32

  int8_t nvm_par_p4 = (int8_t) raw_calibration[10];
  calibration.par_p4 =
      ((double)nvm_par_p4) / 137438953472.0d; // nvm_par_p4 / 2^37

  uint16_t nvm_par_p5 = byte_concat(raw_calibration[12], raw_calibration[11]);
  calibration.par_p5 = ((double)nvm_par_p5) / 0.125d; // nvm_par_p5 / 2^-3

  uint16_t nvm_par_p6 = byte_concat(raw_calibration[14], raw_calibration[13]);
  calibration.par_p6 = ((double)nvm_par_p6) / 64.0d; // nvm_par_p6 / 2^6

  int8_t nvm_par_p7 = (int8_t) raw_calibration[15];
  calibration.par_p7 = ((double)nvm_par_p7) / 256.0d; // nvm_par_p7 / 2^8

  int8_t nvm_par_p8 = (int8_t) raw_calibration[16];
  calibration.par_p8 = ((double)nvm_par_p8) / 32768.0d; // nvm_par_p8 / 2^15

  int16_t nvm_par_p9 = (int16_t) byte_concat(raw_calibration[18], raw_calibration[17]);
  calibration.par_p9 =
      ((double)nvm_par_p9) / 281474976710656.0d; // nvm_par_p9 / 2^48

  int8_t nvm_par_p10 = (int8_t) raw_calibration[19];
  calibration.par_p10 =
      ((double)nvm_par_p10) / 281474976710656.0d; // nvm_par_p10 / 2^48

  int8_t nvm_par_p11 = (int8_t) raw_calibration[20];
  calibration.par_p11 =
      ((double)nvm_par_p11) / 36893488147419103232.0d; // nvm_par_p11 / 2^65
}

static void enable_and_set_mode(bool pressure, bool temperature,
                                bmp388_mode_t mode) {
  uint8_t value = 0;
  value = bmp388_read_register(BMP388_REG_PWR_CTRL);
  if (pressure) {
    value |= (1 << 0); // set bit 0
  } else {
    // TODO clear bit
  }
  if (temperature) {
    value |= (1 << 1); // set bit 1
  } else {
    // TODO clear bit
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

static void bmp388_read_registers(uint8_t address, uint8_t *data,
                                  uint8_t length) {
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
