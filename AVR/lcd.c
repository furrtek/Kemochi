#include "main.h"
#include "lcd.h"
#include "exee.h"

extern uint8_t xpos;

void lcd_tx(uint8_t v) {
	uint8_t bit;

	PORTA &= ~_BV(LCD_CE);
	_delay_us(2);
	for (bit = 0; bit < 8; bit++) {
		if ((v << bit) & 0x80)
			PORTA |= _BV(LCD_DIN);
		else
			PORTA &= ~_BV(LCD_DIN);
		_delay_us(2);
		PORTA |= _BV(LCD_CLK);
		_delay_us(2);
		PORTA &= ~_BV(LCD_CLK);
	}
	_delay_us(2);
	PORTA |= _BV(LCD_CE);
	_delay_us(2);
}

void lcd_clear() {
	uint8_t c;

	PORTA |= _BV(LCD_DC);
	for (c = 0; c < 252; c++) {
		lcd_tx(0);
		lcd_tx(0);
	}
}

void lcd_txt(char * txt, uint8_t maxlen) {
	uint8_t c, ch, i = 0;

	while ((ch = (*txt++)) && (i < maxlen)) {
		if (ch == 32) {
			// Space
			for (c = 0; c < 5; c++)
				lcd_tx(0);
		} else {
			// Remap
			if (ch == 33) {
				ch = 11 * 5;
			} else if (ch == 63) {
				ch = 12 * 5;
			} else {
				ch = (ch - 0x30) * 5;
			}
			// Draw
			for (c = 0; c < 5; c++)
				lcd_tx(exee_read_byte(128 + ch + c));
		}
		lcd_tx(0);	// Remove ?
		i++;
	}
}

void lcd_xy(uint8_t x, uint8_t y) {
	PORTA &= ~_BV(LCD_DC);
	lcd_tx(0x80 | x);
	lcd_tx(0x40 | y);
	PORTA |= _BV(LCD_DC);
}

void lcd_draw(uint8_t x, uint8_t y, uint8_t s, uint8_t e, uint8_t inv, uint8_t flip) {
	uint8_t b;

	lcd_xy(x, y);

	if (!flip) {
		for (b = s; b < e + s; b++)
			lcd_tx(exee_buf[b] ^ inv);
	} else {
		for (b = e + s; b > s; b--)
			lcd_tx(exee_buf[b] ^ inv);
	}
}

void drawfur(uint8_t frame, uint8_t flip) {
	exee_read_page(frame);
	lcd_draw(xpos, 1, 0, 32, 0, flip);
	lcd_draw(xpos, 2, 32, 32, 0, flip);
	exee_read_page(frame+1);
	lcd_draw(xpos, 3, 0, 32, 0, flip);
	lcd_draw(xpos, 4, 32, 32, 0, flip);
}
