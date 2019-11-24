/*
 * bmp388.h
 *
 * Created: 9/16/2019 9:24:42 PM
 *  Author: Wesley
 */

#ifndef BMP388_H_
#define BMP388_H_

typedef struct bmp_reading {
	double temperature;
	double pressure;
} bmp_reading;

void bmp388_reset(void);
void bmp388_init(void);
void bmp388_get_reading(bmp_reading*);



#endif /* BMP388_H_ */
