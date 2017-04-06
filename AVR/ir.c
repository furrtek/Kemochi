#include "main.h"
#include "exee.h"
#include "i2c.h"
#include "lcd.h"
#include "ir.h"

void ir_decode() {
	uint8_t c, checksum, cmd, len;
	player_info_A * p_info = (player_info_A*)exee_buf;

	if (ir_rx_buf[0] != IR_MAGIC)
		return;		// Invalid magic byte

	checksum = 0xAA;
	for (c = 1; c <= ir_rx_buf[2]; c++)
		checksum += ir_rx_buf[c];

	if (checksum != ir_rx_buf[c])	// + 1 ?
		return;		// Invalid checksum

	// Beep
	PCMSK1 = 0b00000010;
	DDRB |= _BV(BTN_A);
	for (c = 0; c < 32; c++) {
		PORTB ^= _BV(BTN_A);
		_delay_us(200);
	}
	DDRB &= ~_BV(BTN_A);
	PORTB |= 1;
	PCMSK1 = 0b00000011;

	cmd = ir_rx_buf[1];
	len = ir_rx_buf[2];

	if (cmd == IR_CMD_PING) {
		// Todo

	} else if (cmd == IR_CMD_FLASHPROG) {
		// Todo

	} else if (cmd == IR_CMD_EEPROG) {
		exee_set_address((ir_rx_buf[3] << 8) | ir_rx_buf[4]);
		i2c_write(EE_WRITE);
		for (c = 0; c < len - 2; c++)
			i2c_write(ir_rx_buf[c + 5]);
		i2c_stop();
	} else if (cmd == IR_CMD_SPECID) {
		// Compare species
		exee_read_page(EEP_PINFO_A);
		for (c = 0; c < 12; c++)
			if (p_info->name[c] != ir_rx_buf[c + 3])
				break;
		
		if (c == 12) {
			// Same !
			lcd_clear();

			lcd_xy(6, 2);
			lcd_txt("HEY! ANOTHER", 16);
		} else {
			// Different
			lcd_clear();

			lcd_xy(6, 2);
			lcd_txt("RMT IS A", 16);
		}
		lcd_xy(12, 3);
		lcd_txt(&p_info->name[0], 12);
	}
}
