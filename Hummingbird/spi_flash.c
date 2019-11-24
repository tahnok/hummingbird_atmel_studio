/*
 * spi_flash.c
 *
 * Created: 11/23/2019 8:57:02 PM
 *  Author: Wesley
 */ 
#include "atmel_start.h"
#include "atmel_start_pins.h"
#include "error.h"
#include <stdint.h>

static void spi_flash_transfer(uint8_t *write, uint8_t write_len, uint8_t *read, uint8_t read_len);

static const uint8_t W25_CMD_POWER_ON[] = {
	0xAB,
	0x00,
	0x00,
	0x00,
};
static const uint8_t W25_CMD_READ_MANUFACTER_ID[] = {0x90, 0x00, 0x00};
static const uint8_t W25_CMD_READ_JEDEC_ID[] = {0x9F};

static const uint8_t JEDEC_ID[] = {0xEF, 0x70, 0x17};
static const uint8_t MANUFACTURER_ID[] = {0x00, 0x16, 0xEF };
static const uint8_t ID = 0x16;


static struct io_descriptor *io;


void spi_flash_init(void) {
	spi_m_sync_get_io_descriptor(&SPI_0, &io);
	spi_m_sync_enable(&SPI_0);

	uint8_t result;
	spi_flash_transfer(W25_CMD_POWER_ON, sizeof(W25_CMD_POWER_ON), &result, 1);
	if (result != ID) {
		error(SPI_FLASH_INIT_FAIL);
	}
	
	uint8_t manufacuter_id[3];
	spi_flash_transfer(W25_CMD_READ_MANUFACTER_ID, sizeof(W25_CMD_READ_MANUFACTER_ID), manufacuter_id, sizeof(manufacuter_id));
	for(int i = 0; i < sizeof(MANUFACTURER_ID); i++) {
		if(manufacuter_id[i] != MANUFACTURER_ID[i]) {
			error(SPI_FLASH_INIT_FAIL);
		}
	}
	
	uint8_t jedec_id[3];
	spi_flash_transfer(W25_CMD_READ_JEDEC_ID, sizeof(W25_CMD_READ_JEDEC_ID), jedec_id, sizeof(jedec_id));
	for(int i = 0; i < sizeof(JEDEC_ID); i++) {
		if(jedec_id[i] != JEDEC_ID[i]) {
			error(SPI_FLASH_INIT_FAIL);
		}
	}
}

static void spi_flash_transfer(uint8_t *write, uint8_t write_len, uint8_t *read, uint8_t read_len) {
	gpio_set_pin_level(FLASH_CS, false);
	io_write(io, write, write_len);
	io_read(io, read, read_len);
	gpio_set_pin_level(FLASH_CS, true);
}
