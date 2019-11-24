/*
 * error.h
 *
 * Created: 9/16/2019 8:38:26 PM
 *  Author: Wesley
 */

#ifndef ERROR_H_
#define ERROR_H_

typedef enum ERROR_REASON {
    RFM95_INIT_FAIL,
	BMP388_INIT_FAIL,
	SPI_FLASH_INIT_FAIL,
} ERROR_REASON;

void error(ERROR_REASON reason);

#endif /* ERROR_H_ */
