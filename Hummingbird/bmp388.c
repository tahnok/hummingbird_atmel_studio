/*
 * bmp388.c
 *
 * Created: 9/16/2019 9:25:01 PM
 *  Author: Wesley
 */ 

#include "atmel_start.h"
#include "atmel_start_pins.h"
#include "error.h"
#include <stdint.h>

static uint8_t spi_read_register(struct io_descriptor *const io, uint8_t address);

static const uint8_t READ_MASK = 0x80; // Set bit 7 high for read

static const uint8_t BMP388_REG_CHIP_ID = 0x00;

void bmp388_init() {
		struct io_descriptor *io;
		spi_m_sync_get_io_descriptor(&SPI_2, &io);

		spi_m_sync_enable(&SPI_2);
		volatile uint8_t result = spi_read_register(io, BMP388_REG_CHIP_ID);
		if (result != 0x50) {
			error(BMP388_INIT_FAIL);
		}
}

static uint8_t spi_read_register(struct io_descriptor *const io, uint8_t address) {
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
