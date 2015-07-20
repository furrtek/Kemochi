#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include "exee.h"

void i2c_io_set_sda(uint8_t hi) {
	if (hi) {
		DDRA &= ~_BV(SDA);
		PORTA &= ~_BV(SDA);
	} else {
		DDRA |= _BV(SDA);
		PORTA &= ~_BV(SDA);
	}
}

void i2c_io_set_scl(uint8_t hi) {
	if (hi)
		PORTA |= _BV(SCL);
	else
		PORTA &= ~_BV(SCL);
}

void i2c_start() {
  	i2c_io_set_scl(1);
	_delay_us(2);
  	i2c_io_set_sda(0);
	_delay_us(2);
  	i2c_io_set_scl(0);
	_delay_us(10);
}

void i2c_stop() {
	_delay_us(2);
  	i2c_io_set_scl(0);
	_delay_us(2);
  	i2c_io_set_sda(0);
	_delay_us(2);
  	i2c_io_set_scl(1);
	_delay_us(2);
  	i2c_io_set_sda(1);
}

uint8_t i2c_write(uint8_t byte) {
	uint8_t ack, bit;

	for (bit=0;bit<8;bit++) {
		if (((byte<<bit) & 0x80) == 0x80)
			i2c_io_set_sda(1);
		else
			i2c_io_set_sda(0);
		_delay_us(2);
  		i2c_io_set_scl(1);
		_delay_us(2);
  		i2c_io_set_scl(0);
	}
	i2c_io_set_sda(1);
	_delay_us(5);

  	i2c_io_set_scl(1);
	_delay_us(2);
	ack = (PINA & SDA)>>SDA;
	_delay_us(2);
  	i2c_io_set_scl(0);
	_delay_us(2);
	return ack;
}

uint8_t i2c_read() {
	uint8_t byte = 0x00, bit;
	for (bit=0;bit<8;bit++) {
  		i2c_io_set_sda(1);
    	i2c_io_set_scl(0);
		_delay_us(2);
    	i2c_io_set_scl(1);
		_delay_us(2);
		byte <<= 1;
    	if (PINA & _BV(SDA)) byte |= 1;
		_delay_us(1);
    	i2c_io_set_scl(0);
	}
	return byte;
}

void i2c_readack() {
   	i2c_io_set_scl(0);
	_delay_us(2);
  	i2c_io_set_sda(0);
	_delay_us(2);
   	i2c_io_set_scl(1);
	_delay_us(2);
   	i2c_io_set_scl(0);
	_delay_us(2);
  	i2c_io_set_sda(1);
	_delay_us(2);
}

void i2c_noreadack() {
   	i2c_io_set_scl(0);
	_delay_us(2);
  	i2c_io_set_sda(1);
	_delay_us(2);
   	i2c_io_set_scl(1);
	_delay_us(2);
   	i2c_io_set_scl(0);
	_delay_us(5);
}

uint8_t exee_read_byte(uint16_t addr) {
	uint8_t v;
	i2c_start();
	i2c_write(0xA0);
	i2c_write(addr>>8);
	i2c_write(addr & 0xFF);
	i2c_start();
	i2c_write(0xA1);
	v = i2c_read();
	i2c_noreadack();
	return v;
}

uint8_t eebuf[64];

void exee_read_buf(uint16_t addr) {
	uint8_t c;

	addr *= 64;
	i2c_start();
	i2c_write(0xA0);
	i2c_write(addr>>8);
	i2c_write(addr & 0xFF);
	i2c_start();
	i2c_write(0xA1);
	for (c=0;c<64;c++) {
		eebuf[c] = i2c_read();
		if (c == 63)
			i2c_noreadack();
		else
			i2c_readack();
	}
}
