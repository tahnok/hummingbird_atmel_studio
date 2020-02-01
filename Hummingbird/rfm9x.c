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

static void spi_write_register(uint8_t address, uint8_t value);
static uint8_t spi_read_register(uint8_t address);
static void rfm9x_set_frequency(float);
static void rfm9x_set_power(uint8_t);

static const uint8_t WNR_MASK = 0x80;

// Registers
static const uint8_t RFM95_REG_FIFO = 0x00;
static const uint8_t RFM95_REG_OP_MODE = 0x01;
static const uint8_t RFM95_REG_FRF_MSB = 0x06;
static const uint8_t RFM95_REG_FRF_MID = 0x07;
static const uint8_t RFM95_REG_FRF_LSB = 0x08;
static const uint8_t RFM95_REG_PA_CONFIG = 0x09;
static const uint8_t RFM95_REG_FIFO_ADDRESS = 0x0d;
static const uint8_t RFM95_REG_FIFO_TX_ADDRESS = 0x0e;
static const uint8_t RFM95_REG_MODEM_CONFIG_1 = 0x1d;
static const uint8_t RFM95_REG_MODEM_CONFIG_2 = 0x1e;
static const uint8_t RFM95_REG_PREAMBLE_MSB = 0x20;
static const uint8_t RFM95_REG_PREAMBLE_LSB = 0x21;
static const uint8_t RFM95_REG_PAYLOAD_LENGTH = 0x22;
static const uint8_t RFM95_REG_MODEM_CONFIG_3 = 0x26;
static const uint8_t RFM95_REG_PA_DAC = 0x4d;
// const static uint8_t RFM95_REG_VERSION = 0x42;

// Modes
static const uint8_t OP_MODE_SLEEP = 0x00;      // 000 SLEEP
static const uint8_t OP_MODE_STANDBY = 0x01;    // 001 Standby
static const uint8_t OP_MODE_TX = 0x03;         // 011 Transmit
static const uint8_t OP_MODE_LONG_RANGE = 0x80; // 1000 0000

// Config, defaults to Bw125Cr45Sf128

/*
See page 106 of HopeRF 95/96/97/98(W) manual

signal bandwidth (bits 7-4): 0111 = 125 kHZ
coding rate (bits 3-1): 001 = 4/5
implicit header mode (bit 0): 0 = explicit header mode
*/
static const uint8_t RFM95_CONFIG_1 = 0x72;
/*
See page 107 of HopeRF 95/96/97/98(W) manual

Spreading factor (bits 7-4): 0111 = 128 chips / symbol
TX continuous mode (bit 3): 0 = normal mode
RX Payload CRC (bit 2): 1 = Header CRC on
RX Timeout MSB (bits 1-0): 0 = ??? (I don't think this is used)
*/
static const uint8_t RFM95_CONFIG_2 = 0x74;
/*
See page 107 of HopeRF 95/96/97/98(W) manual

Unused (bits 7-4)
Mobile  node (bit 3): 0 = static node
AGC Auto On (bit 2): 1 =  LNA Gain set by internal AGC loop
Reserved (bits 1-0): 0 = ??
*/
static const uint8_t RFM95_CONFIG_3 = 0x04;

// globals :gulp:

static struct io_descriptor *io;

void rfm9x_init() {
  spi_m_sync_get_io_descriptor(&SPI_1, &io);
  spi_m_sync_enable(&SPI_1);

  gpio_set_pin_level(LORA_RESET, false);
  delay_ms(10);
  gpio_set_pin_level(LORA_RESET, true);
  delay_ms(10);

  uint8_t new_mode = OP_MODE_SLEEP | OP_MODE_LONG_RANGE;
  spi_write_register(RFM95_REG_OP_MODE, new_mode);

  uint8_t mode = spi_read_register(RFM95_REG_OP_MODE);
  if (mode != new_mode) {
    error(RFM95_INIT_FAIL);
  }

  // set up fifo for TX to use whole internal stack
  spi_write_register(RFM95_REG_FIFO_TX_ADDRESS, 0);

  // set mode to idle
  spi_write_register(RFM95_REG_OP_MODE, OP_MODE_STANDBY);

  // set modem config Bandwidth: 125, Coding Rate: 4/5, Spreading Factor
  // 128, AGC enabled
  spi_write_register(RFM95_REG_MODEM_CONFIG_1, RFM95_CONFIG_1);
  spi_write_register(RFM95_REG_MODEM_CONFIG_2, RFM95_CONFIG_2);
  spi_write_register(RFM95_REG_MODEM_CONFIG_3, RFM95_CONFIG_3);

  // set preamble to 8
  spi_write_register(RFM95_REG_PREAMBLE_MSB, 0);
  spi_write_register(RFM95_REG_PREAMBLE_LSB, 8);

  // set frequency magic with FRF registers
  rfm9x_set_frequency(915.0);

  // disable PA since I don't think I need lots of power

  // set power to 13,
  rfm9x_set_power(13);
}

void rfm9x_send(uint8_t *data, uint8_t length) {
  // wait for packet sent?
  // set mode to standby
  spi_write_register(RFM95_REG_OP_MODE, OP_MODE_STANDBY);
  // wait for Channel Activity Detected to be false?

  // set the FIFO to 0
  spi_write_register(RFM95_REG_FIFO_ADDRESS, 0);

  // sending is done by writing to the RH_RF95_REG_00_FIFO register

  // send 4 "header" bytes for radiohead compatibility
  // header to (default to 0xff)
  spi_write_register(RFM95_REG_FIFO, 0xff);
  // header from (default to 0xff)
  spi_write_register(RFM95_REG_FIFO, 0xff);
  // header id (default 0x00)
  spi_write_register(RFM95_REG_FIFO, 0x00);
  // header flags (default 0x00)
  spi_write_register(RFM95_REG_FIFO, 0x00);

  // write data to FIFO register
  for (uint8_t i = 0; i < length; i++) {
    spi_write_register(RFM95_REG_FIFO, data[i]);
  }

  // write payload len to RH_RF95_REG_22_PAYLOAD_LENGTH (which is length + 4)
  spi_write_register(RFM95_REG_PAYLOAD_LENGTH, length + 4);

  // set the mode to TX
  spi_write_register(RFM95_REG_OP_MODE, OP_MODE_TX);
}

/*
input: frequency in MHz

transforms and writes frequency into FRF registers
*/
static void rfm9x_set_frequency(float frequency) {
  // crystal freq 32 * 100000
  // 2^19 = 524288
  // F_step = 3200000 / 524288
  float f_step = 61.03515625;
  uint32_t frf = (frequency * 1000000.0f) / f_step;
  spi_write_register(RFM95_REG_FRF_MSB, (frf >> 16) & 0xff);
  spi_write_register(RFM95_REG_FRF_MID, (frf >> 8) & 0xff);
  spi_write_register(RFM95_REG_FRF_LSB, frf & 0xff);
}

static void rfm9x_set_power(uint8_t power_level) {
  // if power level over over 20, need to set RFM95_REG_PA_DAC to 0x87
  // ref: page 79
  if ((power_level < 5) || (power_level > 20)) {
    error(RFM95_INVALID_POWER);
  }
  const uint8_t USE_PA_BOOST = 0x80; // use PA_BOOST output pin, page 88
  uint8_t new_reg_value =
      USE_PA_BOOST | (power_level - 2); // datasheet says subtract 2
  spi_write_register(RFM95_REG_PA_CONFIG, new_reg_value);
}

static void spi_write_register(uint8_t address, uint8_t value) {
  uint8_t data[2];
  data[0] = address | WNR_MASK; // must set wnr bit (7th bit) to 1 for write
  data[1] = value;
  gpio_set_pin_level(LORA_CS, false);
  io_write(io, (uint8_t *)data, sizeof(data));
  gpio_set_pin_level(LORA_CS, true);
}

static uint8_t spi_read_register(uint8_t address) {
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


