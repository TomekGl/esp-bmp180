/*
 * bmp180.h
 *
 *  Created on: Oct 30 2016
 *      Author: Tomasz Głuch
 */

#ifndef INCLUDE_DRIVER_BMP180_H_
#define INCLUDE_DRIVER_BMP180_H_

#define BMP180_ADDR 0xee

void LOCAL ICACHE_FLASH_ATTR bmp180_reset(void);
int16_t ICACHE_FLASH_ATTR bmp_get_temp(void);
uint32_t ICACHE_FLASH_ATTR bmp_get_pressure(uint8_t oss);
uint8 ICACHE_FLASH_ATTR bmp180_init(void);

#endif							/* INCLUDE_DRIVER_BMP180_H_ */
