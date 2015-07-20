#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include "exee.h"

extern uint8_t xpos;

void txlcd(uint8_t b) {
	uint8_t c;

	PORTA &= ~_BV(PA2);
	_delay_us(2);
	for (c=0;c<8;c++) {
		if ((b<<c)&0x80)
			PORTA |= _BV(PA5);
		else
			PORTA &= ~_BV(PA5);
		_delay_us(2);
		PORTA |= _BV(PA4);
		_delay_us(2);
		PORTA &= ~_BV(PA4);
	}
	_delay_us(2);
	PORTA |= _BV(PA2);
	_delay_us(2);
}

void clrlcd() {
	uint8_t c;
	PORTA |= 0b00001000;
	for (c=0;c<252;c++) {
		txlcd(0);
		txlcd(0);
	}
}

void lcdtxt(char *txt) {
	uint8_t tc,cv;
	while (*txt) {
		cv = (*txt);
		if (cv == 32) {
			for (tc=0;tc<5;tc++)
				txlcd(0);
		} else {
			if (cv == 33) {
				cv = 11*5;
			} else if (cv == 63) {
				cv = 12*5;
			} else {
				cv = (cv-0x30)*5;
			}
			for (tc=0;tc<5;tc++)
				txlcd(exee_read_byte(128+cv+tc));
		}
		txlcd(0);
		txt++;
	}
}

void lcdxy(uint8_t x, uint8_t y) {
	PORTA &= ~0b00001000;
	txlcd(0x80+x);
	txlcd(0x40+y);
	PORTA |= 0b00001000;
}

void drawlcd(uint8_t x, uint8_t y, uint8_t s, uint8_t e, uint8_t inv, uint8_t flip) {
	uint8_t b;

	lcdxy(x,y);
	if (!flip) {
		for (b=s;b<e+s;b++) {
			txlcd(eebuf[b] ^ inv);
		}
	} else {
		for (b=e+s;b>s;b--) {
			txlcd(eebuf[b] ^ inv);
		}

	}
}

void drawfur(uint8_t frame, uint8_t flip) {
	exee_read_buf(frame);
	drawlcd(xpos,1,0,32,0,flip);
	drawlcd(xpos,2,32,32,0,flip);
	exee_read_buf(frame+1);
	drawlcd(xpos,3,0,32,0,flip);
	drawlcd(xpos,4,32,32,0,flip);
}
