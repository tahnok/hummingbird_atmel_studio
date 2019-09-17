/*
 * rfm9x.c
 *
 * Created: 9/14/2019 6:59:33 PM
 *  Author: Wesley
 */

#include "rfm9x.h"
#include "atmel_start.h"
#include "atmel_start_pins.h"
#include "error.h"

static void spi_write_register(struct io_descriptor *const io, uint8_t address,
                               uint8_t value);
static uint8_t spi_read_register(struct io_descriptor *const io,
                                 uint8_t address);

static const uint8_t WNR_MASK = 0x80;

// Registers
static const uint8_t RFM95_REG_OP_MODE = 0x01;
// const static uint8_t RFM95_REG_VERSION = 0x42;

// Modes
static const uint8_t OP_MODE_SLEEP = 0x0;       // 000 SLEEP
static const uint8_t OP_MODE_LONG_RANGE = 0x80; // 1000 0000

void rfm9x_init() {
  struct io_descriptor *io;
  spi_m_sync_get_io_descriptor(&SPI_1, &io);
  spi_m_sync_enable(&SPI_1);

  uint8_t new_mode = OP_MODE_SLEEP | OP_MODE_LONG_RANGE;
  spi_write_register(io, RFM95_REG_OP_MODE, new_mode);

  uint8_t mode = spi_read_register(io, RFM95_REG_OP_MODE);
  if (mode != new_mode) {
	  error(RFM95_INIT_FAIL);
  }
}

static void spi_write_register(struct io_descriptor *const io, uint8_t address,
                               uint8_t value) {
  uint8_t data[2];
  data[0] = address | WNR_MASK; // must set wnr bit (7th bit) to 1 for write
  data[1] = value;
  gpio_set_pin_level(LORA_CS, false);
  io_write(io, (uint8_t *)data, sizeof(data));
  gpio_set_pin_level(LORA_CS, true);
}

static uint8_t spi_read_register(struct io_descriptor *const io,
                                 uint8_t address) {
  address = address & ~WNR_MASK;
  uint8_t result;
  gpio_set_pin_level(LORA_CS, false);
  io_write(io, &address, 1);
  io_read(io, &result, 1);
  gpio_set_pin_level(LORA_CS, true);
  return result;
}

/*
const uint8_t RFM95_REG_OP_MODE[] = {0x01};
const uint8_t RFM95_REG_VERSION[] = {0x42};
const uint8_t OP_MODE_SLEEP = 0x0;
const uint8_t OP_MODE_LORA = 0x80;

void test_rfm9x() {
  struct io_descriptor *io;
  spi_m_sync_get_io_descriptor(&SPI_1, &io);
  spi_m_sync_enable(&SPI_1);

  gpio_set_pin_level(LORA_CS, false);
  io_write(io, RFM95_REG_VERSION, sizeof(RFM95_REG_VERSION));
  uint8_t version;
  io_read(io, &version, 1);
  gpio_set_pin_level(LORA_CS, true);
  this is returning 12, which is weird, the datasheet says it should be 11?
hoping new version...


  gpio_set_pin_level(LORA_CS, false);
  io_write(io, RFM95_REG_OP_MODE, sizeof(RFM95_REG_OP_MODE));
  uint8_t mode[] = {OP_MODE_SLEEP | 0x80 };
  io_write(io, mode, 1);
  gpio_set_pin_level(LORA_CS, true);
  delay_ms(10);

  gpio_set_pin_level(LORA_CS, false);
  io_write(io, RFM95_REG_OP_MODE, sizeof(RFM95_REG_OP_MODE));
  uint8_t actual_mode;
  io_read(io, &actual_mode, 1);
  gpio_set_pin_level(LORA_CS, true);
  //not going into sleep mode... of 0x00
}

*/

