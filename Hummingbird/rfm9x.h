/*
 * rfm9x.h
 *
 * Created: 9/16/2019 8:11:48 PM
 *  Author: Wesley
 */ 


#ifndef RFN9X_H_
#define RFN9X_H_

#include <stdbool.h>
#include <stdint.h>

void rfm9x_init(void);
void rfm9x_send(uint8_t *data, uint8_t length);

#endif /* RFN9X_H_ */
