#include "main.h"
#include "i2c.h"

void i2c_set_sda(uint8_t v) {
	if (v) {
		DDRA &= ~_BV(I2C_SDA);
		PORTA &= ~_BV(I2C_SDA);
	} else {
		DDRA |= _BV(I2C_SDA);
		PORTA &= ~_BV(I2C_SDA);
	}
}

void i2c_set_scl(uint8_t v) {
	if (v)
		PORTA |= _BV(I2C_SCL);
	else
		PORTA &= ~_BV(I2C_SCL);
}

void i2c_start() {
  	i2c_set_scl(1);
	_delay_us(2);
  	i2c_set_sda(0);
	_delay_us(2);
  	i2c_set_scl(0);
	_delay_us(10);
}

void i2c_stop() {
	_delay_us(2);
  	i2c_set_scl(0);
	_delay_us(2);
  	i2c_set_sda(0);
	_delay_us(2);
  	i2c_set_scl(1);
	_delay_us(2);
  	i2c_set_sda(1);
}

uint8_t i2c_write(uint8_t byte) {
	uint8_t ack, bit;

	for (bit = 0; bit < 8; bit++) {
		if ((byte << bit) & 0x80)
			i2c_set_sda(1);
		else
			i2c_set_sda(0);
		_delay_us(2);
  		i2c_set_scl(1);
		_delay_us(2);
  		i2c_set_scl(0);
	}
	i2c_set_sda(1);
	_delay_us(5);

  	i2c_set_scl(1);
	_delay_us(2);
	ack = (PINA & I2C_SDA) >> I2C_SDA;
	_delay_us(2);
  	i2c_set_scl(0);
	_delay_us(2);

	return ack;
}

uint8_t i2c_read() {
	uint8_t byte = 0x00, bit;

	for (bit = 0; bit < 8; bit++) {
  		i2c_set_sda(1);
    	i2c_set_scl(0);
		_delay_us(2);
    	i2c_set_scl(1);
		_delay_us(2);

		byte <<= 1;
    	if (PINA & _BV(I2C_SDA)) byte |= 1;

		_delay_us(1);
    	i2c_set_scl(0);
	}

	return byte;
}

void i2c_readack() {
   	i2c_set_scl(0);
	_delay_us(2);
  	i2c_set_sda(0);
	_delay_us(2);
   	i2c_set_scl(1);
	_delay_us(2);
   	i2c_set_scl(0);
	_delay_us(2);
  	i2c_set_sda(1);
	_delay_us(2);
}

void i2c_noreadack() {
   	i2c_set_scl(0);
	_delay_us(2);
  	i2c_set_sda(1);
	_delay_us(2);
   	i2c_set_scl(1);
	_delay_us(2);
   	i2c_set_scl(0);
	_delay_us(5);
}
