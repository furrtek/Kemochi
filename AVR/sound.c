#include "main.h"
#include "sound.h"

void selbeep() {
	uint8_t c;

	PCMSK1 = 0b00000010;
	DDRB |= 1;
	for (c = 0; c < 64; c++) {
		PORTB ^= 1;
		_delay_us(400);
	}
	DDRB &= ~1;
	PORTB |= 1;
}

void valbeep() {
	uint8_t c;

	PCMSK1 = 0b00000010;
	DDRB |= 1;
	for (c=0;c<64;c++) {
		PORTB ^= 1;
		_delay_us(200);
	}
	DDRB &= ~1;
	PORTB |= 1;
}
