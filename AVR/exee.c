#include "main.h"
#include "i2c.h"
#include "exee.h"

void exee_set_address(uint16_t addr) {
	i2c_start();
	i2c_write(EE_WRITE);
	i2c_write(addr >> 8);
	i2c_write(addr & 0xFF);
}

uint8_t exee_read_byte(uint16_t addr) {
	uint8_t v;

	exee_set_address(addr);
	i2c_start();
	i2c_write(EE_READ);
	v = i2c_read();
	i2c_noreadack();

	return v;
}

void exee_read_page(uint16_t page) {
	uint8_t c;

	exee_set_address(page * 64);
	i2c_start();
	i2c_write(EE_READ);

	for (c = 0; c < 64; c++) {
		exee_buf[c] = i2c_read();
		if (c == 63)
			i2c_noreadack();
		else
			i2c_readack();
	}

	exee_last_page = page;
}
