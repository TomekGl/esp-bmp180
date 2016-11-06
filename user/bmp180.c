/*
* bmp180.c
*
*  Created on: Oct 30 2016
*      Author: Tomasz Głuch
*/

#include "esp8266.h"
#include "i2c_master.h"
#include "bmp180.h"

#ifndef DEBUG
//#define DEBUG
#endif

/* Global variables */
long b5 = 0;

union {
	struct {
		int16_t ac1;
		int16_t ac2;
		int16_t ac3;
		uint16_t ac4;
		uint16_t ac5;
		uint16_t ac6;
		int16_t b1;
		int16_t b2;
		int16_t mb;
		int16_t mc;
		int16_t md;
	} val __attribute__ ((packed));
	uint8_t bytes[22];
	uint16_t words[11];
} coeff_data;


/* Private definitions */
void ICACHE_FLASH_ATTR bmp180_swap_bytes(void *ptr);
uint8_t ICACHE_FLASH_ATTR bmp180_get_cal_param();
int16_t ICACHE_FLASH_ATTR bmp180_get_ut();
uint32_t ICACHE_FLASH_ATTR bmp180_get_up(uint8_t oss);



void LOCAL ICACHE_FLASH_ATTR bmp180_reset(void)
{
	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR);
	i2c_master_getAck();
	i2c_master_writeByte(0xe0);
	i2c_master_getAck();
	i2c_master_writeByte(0xb6);
	i2c_master_getAck();
	i2c_master_stop();
}

void ICACHE_FLASH_ATTR bmp180_swap_bytes(void *ptr)
{
	uint8_t tmp;
	uint8_t *bytes;

	bytes = (uint8_t *) ptr;
	tmp = *bytes;
	*(bytes) = *(bytes + 1);
	*(bytes + 1) = tmp;
}

uint8_t ICACHE_FLASH_ATTR bmp180_get_cal_param()
{
	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR);
	i2c_master_getAck();
	i2c_master_writeByte(0xAA);
	i2c_master_getAck();
	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR | 1);
	i2c_master_getAck();

	uint8_t i;
	for (i = 0; i < (sizeof(coeff_data.bytes) - 1); i++) {
		coeff_data.bytes[i] = i2c_master_readByte();
		i2c_master_send_ack();
	}
	coeff_data.bytes[i] = i2c_master_readByte();
	i2c_master_send_nack();
	i2c_master_stop();

	bmp180_swap_bytes(&coeff_data.val.ac1);
	bmp180_swap_bytes(&coeff_data.val.ac2);
	bmp180_swap_bytes(&coeff_data.val.ac3);
	bmp180_swap_bytes(&coeff_data.val.ac4);
	bmp180_swap_bytes(&coeff_data.val.ac5);
	bmp180_swap_bytes(&coeff_data.val.ac6);
	bmp180_swap_bytes(&coeff_data.val.b1);
	bmp180_swap_bytes(&coeff_data.val.b2);
	bmp180_swap_bytes(&coeff_data.val.mb);
	bmp180_swap_bytes(&coeff_data.val.mc);
	bmp180_swap_bytes(&coeff_data.val.md);

#ifdef DEBUG
	os_printf("BMP180: Calibration coefficients\r\n");
	os_printf("ac1: %d, ac2: %d, ac3: %d\r\n", coeff_data.val.ac1,
			  coeff_data.val.ac2, coeff_data.val.ac3);
	os_printf("ac4: %d, ac5: %d, ac6: %d\r\n", coeff_data.val.ac4,
			  coeff_data.val.ac5, coeff_data.val.ac6);
	os_printf("b1:  %d, b2: %d\r\n", coeff_data.val.b1, coeff_data.val.b2);
	os_printf("mb:  %d, mc: %d, md: %d\r\n", coeff_data.val.mb,
			  coeff_data.val.mc, coeff_data.val.md);
#endif

// Validate coefficients - 0x0000 or 0xffff means error
	for (i = 0;
		 i <
		 ((sizeof(coeff_data.words) / sizeof(coeff_data.words[0])) - 1);
		 i++) {
		if (coeff_data.words[i] == 0 || coeff_data.words[i] == 0xffff)
			return 0;
	}

	return 1;
}

int16_t ICACHE_FLASH_ATTR bmp180_get_ut()
{
	int16_t ut;
	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR);
	i2c_master_getAck();
	i2c_master_writeByte(0xf4);
	i2c_master_getAck();
	i2c_master_writeByte(0x2e);
	i2c_master_getAck();
	i2c_master_stop();

	os_delay_us(5000L);			//5ms

	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR);
	i2c_master_getAck();
	i2c_master_writeByte(0xf6);
	i2c_master_getAck();
	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR | 1);
	i2c_master_getAck();

	ut = i2c_master_readByte() << 8;
	i2c_master_send_ack();
	ut += i2c_master_readByte();
	i2c_master_send_nack();
	i2c_master_stop();

	return ut;

}

int16_t ICACHE_FLASH_ATTR bmp180_get_temp()
{

	long x1, x2, t, ut;
/* b5 is global
*   long b5;
*/

	ut = bmp180_get_ut();
	x1 = ((long) ut -
		  (long) coeff_data.val.ac6) * (long) coeff_data.val.ac5 >> 15;
	x2 = ((long) coeff_data.val.mc << 11) / (x1 + coeff_data.val.md);
	b5 = x1 + x2;
	t = (b5 + 8L) >> 4;

#ifdef DEBUG
	os_printf("BMP180: Calibration coefficients\r\n");
	os_printf("ac1: %d, ac2: %d, ac3: %d\r\n", coeff_data.val.ac1,
			  coeff_data.val.ac2, coeff_data.val.ac3);
	os_printf("ac4: %d, ac5: %d, ac6: %d\r\n", coeff_data.val.ac4,
			  coeff_data.val.ac5, coeff_data.val.ac6);
	os_printf("b1:  %d, b2: %d\r\n", coeff_data.val.b1, coeff_data.val.b2);
	os_printf("mb:  %d, mc: %d, md: %d\r\n", coeff_data.val.mb,
			  coeff_data.val.mc, coeff_data.val.md);
	os_printf("Uncompensated temp: %ld\r\n", ut);
	os_printf("Compensated temp:   %ld\r\n", t);
	os_printf("XXX: x1=%ld, x2=%ld, b5=%ld\r\n", x1, x2, b5);
#endif
	return t;
}

uint32_t ICACHE_FLASH_ATTR bmp180_get_up(uint8_t oss)
{
	uint32_t up;

	if (oss > 3)
		oss = 3;

	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR);
	i2c_master_getAck();
	i2c_master_writeByte(0xf4);
	i2c_master_getAck();
	i2c_master_writeByte(0x34 + (oss << 6));
	i2c_master_getAck();
	i2c_master_stop();

	int delay = 0;
	switch (oss) {
	case 0:
		delay = 4500L;
		break;
	case 1:
		delay = 7500L;
		break;
	case 2:
		delay = 13500L;
		break;
	case 3:
	default:
		delay = 25000L;
		break;
	}
	os_delay_us(delay);

	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR);
	i2c_master_getAck();
	i2c_master_writeByte(0xf6);
	i2c_master_getAck();
	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR | 1);
	i2c_master_getAck();

	up = i2c_master_readByte() << 16;
	i2c_master_send_ack();
	up += i2c_master_readByte() << 8;
	i2c_master_send_ack();
	up += i2c_master_readByte();
	up = up >> (8 - oss);
	i2c_master_send_nack();
	i2c_master_stop();

#ifdef DEBUG
	os_printf("Uncompensated pres: %ld (0x%x)\r\n", up, up);
#endif
	return up;

}

#define BMP180_INVALID 0
#define BMP180_PARAM_MG 3038
#define BMP180_PARAM_MH (-7357)
#define BMP180_PARAM_MI 3791

uint32_t ICACHE_FLASH_ATTR bmp180_get_pressure(uint8_t oss)
{
	int32_t b6, x1, x2, x3, b3, p;
	uint32_t b4, b7;
	uint32_t up;
	up = bmp180_get_up(oss);

	b6 = b5 - 4000;
	x1 = (b6 * b6) >> 12;
	x1 *= (int32_t) coeff_data.val.b2;
	x1 >>= 11;

	x2 = (int32_t) coeff_data.val.ac2 * b6;
	x2 >>= 11;

	x3 = x1 + x2;
	b3 = ((((int32_t) coeff_data.val.ac1 * 4 + x3) << oss) + 2) >> 2;

	x1 = (int32_t) coeff_data.val.ac3 * b6;
	x1 >>= 13;

	x2 = b6 * b6;
	x2 >>= 12;
	x2 *= (int32_t) coeff_data.val.b1;
	x2 >>= 16;

	x3 = x1 + x2 + 2;
	x3 >>= 2;

	b4 = (uint32_t) coeff_data.val.ac4 * (uint32_t) (x3 + 32768);
	b4 >>= 15;

	b7 = (up - b3) * (50000 >> oss);

	if (b4 == 0)
		return BMP180_INVALID;
	if (b7 < 0x80000000) {
		p = (b7 << 1) / b4;
	} else {
		p = (b7 / b4) << 1;
	}

	x1 = p >> 8;
	x1 *= x1;
	x1 = (x1 * BMP180_PARAM_MG) >> 16;
	x2 = (BMP180_PARAM_MH * p) >> 16;
	p = p + (x1 + x2 + BMP180_PARAM_MI) / 16;
#ifdef DEBUG
	os_printf("BMP180: Pressure %ld Pa\r\n", p);
#endif
	return p;
}

uint8 ICACHE_FLASH_ATTR bmp180_init()
{
	i2c_master_init();
	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR);
	i2c_master_getAck();
	i2c_master_writeByte(0xd0);
	i2c_master_getAck();
	i2c_master_start();
	i2c_master_writeByte(BMP180_ADDR | 1);
	i2c_master_getAck();
	uint8_t retval = 0;
	retval = i2c_master_readByte();
	i2c_master_send_nack();
	i2c_master_stop();

	bmp180_get_cal_param();
#ifdef DEBUG
	os_printf("BMP180 initialized\n");
#endif

	return retval;
}
