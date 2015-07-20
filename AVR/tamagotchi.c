//Kemochi - furrtek 2015

//IR packet:
//B1 sccccccc 00llllll d... kkkkkkkk
//s = source (0=programmer, 1=other kemochi)
//c = command
//l = length
//k = checksum (start=0xAA)

//Page 00: 7 icons
//Page 01: 3 icons
//Page 02: Alphabet 0
//Page 03: Alphabet 1
//Page 04: Alphabet 2
//Page 05: Alphabet 3

//Page 08: Player info A
//	00: Name (12)		NAME:
//  0C: Specie (12)		IS A:
//  18: Country (12)	FROM:
//  24: Spoken A (14)	SPEAKS:
//  32: Spoken B (14)

//Page 09: Player info B
//  00: Spoken C (14)
//  0E: Likes A (12)	LIKES:
//  1A: Likes B (12)
//  26: Likes C (12)
//	32: Year of birth (4)

//Page 0A: Player status
//	00: Hunger
//	01: Sleepyness
//	02: Boredom
//	03: Discipline
//	04: Happyness

//Page 10-11: STANDING
//Page 12-13: ON ALL FOURS
//Page 14-15: SLEEPING
//Page 16-17: WCA
//Page 18-19: WCB
//Page 1A-1B: EATA
//Page 1C-1D: EATB

#define P_STANDING	0x10
#define P_FOURS		0x12
#define P_SLEEPING	0x14
#define P_WC		0x16
#define P_EAT		0x1A

//Page F0: Sound effects

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>
#include "lcd.h"
#include "exee.h"

#define AWAKE 0
#define SLEEP 1
#define FA 2

uint8_t get_adc() {
	uint8_t value;
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)); 
	value = ADCH;
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCH;
}

void i2c_start();
void i2c_stop();
uint8_t i2c_write(uint8_t byte);
uint8_t i2c_read();

uint8_t volatile rbit, rs, first, bufidx, state, fatimer;
uint16_t volatile stat_sleepy = 0;
uint16_t volatile xpos = 26, xdir;
uint8_t volatile icon_sel = 0;
uint8_t volatile buf[32];

void refresh_icons() {
	uint8_t v;
	// Draw icons
	exee_read_buf(0);
	if (icon_sel == 0) v = 0xFF; else v = 0;
	drawlcd(0+7,0,0,9,v,0);
	if (icon_sel == 1) v = 0xFF; else v = 0;
	drawlcd(15+7,0,9,9,v,0);
	if (icon_sel == 2) v = 0xFF; else v = 0;
	drawlcd(30+7,0,18,9,v,0);
	if (icon_sel == 3) v = 0xFF; else v = 0;
	drawlcd(45+7,0,27,9,v,0);
	if (icon_sel == 4) v = 0xFF; else v = 0;
	drawlcd(60+7,0,36,9,v,0);
	if (icon_sel == 5) v = 0xFF; else v = 0;
	drawlcd(0+7,5,45,9,v,0);
	if (icon_sel == 6) v = 0xFF; else v = 0;
	drawlcd(15+7,5,54,9,v,0);
	exee_read_buf(1);
	if (icon_sel == 7) v = 0xFF; else v = 0;
	drawlcd(30+7,5,0,9,v,0);
	if (icon_sel == 8) v = 0xFF; else v = 0;
	drawlcd(45+7,5,9,9,v,0);
	if (icon_sel == 9) v = 0xFF; else v = 0;
	drawlcd(60+7,5,18,9,v,0);
}

void selbeep() {
	uint8_t c;
	PCMSK1 = 0b00000010;
	DDRB |= 1;
	for (c=0;c<64;c++) {
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

ISR(PCINT0_vect) {
	_delay_ms(5);
	if (!(PINA & _BV(PA1))) {
		valbeep();
	}
	if (!(PINB & _BV(PB0))) {
		if (icon_sel == 9)
			icon_sel = 0;
		else
			icon_sel++;
		refresh_icons();
		selbeep();
	}
	if (!(PINB & _BV(PB1))) {
		valbeep();
	}
	_delay_ms(15);
	PCMSK1 = 0b00000011;
	GIFR = 0;
}

ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));

ISR(TIM1_OVF_vect) {
	first = 1;
	bufidx = 0;
	rbit = 0;
}

ISR(INT0_vect) {
	uint16_t v;
	uint8_t c,cs;

	v = TCNT1;
	TCNT1 = 0;

	if (!first) {
		if (v >= 4000) {
			if (v < 7000) {
				if (buf[0] == 0xB1) {
					buf[0] = 0;
					cs = 0xAA;
					for (c=0;c<buf[2];c++) {
						cs += buf[3+c];
					}
					if (cs == buf[3+c]) {
						//Beep
						PCMSK1 =0b00000010;
						DDRB |= 1;
						for (c=0;c<32;c++) {
							PORTB ^= 1;
							_delay_us(200);
						}
						DDRB &= ~1;
						PORTB |= 1;
						PCMSK1 =0b00000011;

						if (buf[1] == 0) {
							//clrlcd();

							/*PORTA = 0b11000000;
							txlcd(0x40);
							txlcd(0x80);
							PORTA = 0b11001000;*/

							i2c_start();
							i2c_write(0xA0);
							i2c_write(buf[3]);
							i2c_write(buf[3+1]);
							for (c=0;c<buf[2]-2;c++) {
								i2c_write(buf[c+3+2]);
								//txlcd(buf[c+3]);
							}
							i2c_stop();
						} else {
							clrlcd();

							/*PORTA = 0b11000000;
							txlcd(0x42);
							txlcd(0x86);
							PORTA = 0b11001000;
							lcdtxt("HEY! ANOTHER");
							PORTA = 0b11000000;
							txlcd(0x43);
							txlcd(0x9A);
							PORTA = 0b11001000;
							lcdtxt("SERGAL");*/
						}
					}
				}
				bufidx = 0;
			} else {
				first = 1;
				bufidx = 0;
				rbit = 0;
			}
		} else { 
			rs = rs >> 2;
			if ((v >= 800) && (v < 1600)) rs |= 0x00;
			if ((v >= 1600) && (v < 2400)) rs |= 0x40;
			if ((v >= 2400) && (v < 3200)) rs |= 0x80;
			if ((v >= 3200) && (v < 4000)) rs |= 0xC0;

			if (rbit == 3) {
				buf[bufidx] = rs;
				bufidx++;
				rbit = 0;
			} else {
				rbit++;
			}
		}
	} else {
		first = 0;
	}
}

int main(void) {
	uint8_t c, b, bc, anim = 0;

	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = 0x00;

	DDRA = 0b01111100;
	DDRB = 0b00000000;
	PORTA = 2;
	PORTB = 3;

	MCUCR = 0b00000010;
	GIMSK = 0b01110000;
	PCMSK1 =0b00000011;
	PCMSK0 =0b00000010;

	TCCR1A = 0b00000000;
	TCCR1B = 0b00000001;
	TIMSK1 = 0b00000001;

	//Reset LCD
	PORTA = 0b11000110;
	_delay_ms(50);
	PORTA = 0b10000110;
	_delay_ms(10);
	PORTA = 0b11000110;
	_delay_ms(50);

	txlcd(0x21);
	txlcd(0xAE);
	txlcd(0x13);
	txlcd(0x20);
	txlcd(0x0C);

	clrlcd();

	ADMUX = 0b10000000;
	ADCSRA = 0b10000100;
	ADCSRB = 0b00010000;
	DIDR0 = 0b00000001;

	refresh_icons();

	//lcdxy(20,3);
	//lcdtxt("0123456789");

	drawfur(P_STANDING,0);

	sei();

	for(;;) {
		b = get_adc();

		//sprintf(mbuf,"ADC0:%u   ",b);
		//lcdtxt(mbuf);

		//Fall asleep: return to center of screen (xpos=26)
		//Then all4s
		//Then sleeping
		//Don't fuck around with Zz icon's position...

		if (state == SLEEP) {
			if (anim & 1) {
				exee_read_buf(1);
				if (xpos >= 10)
					drawlcd(xpos-10,2,18+((anim&2)>>1)*9,9,0,0);
				else
					drawlcd(xpos+32+1,2,18+((anim&2)>>1)*9,9,0,0);
			}
			if (stat_sleepy) stat_sleepy--;
		}

		if (state == AWAKE) {
			if ((anim & 3) == 0) drawfur(P_STANDING,xdir);
			if ((anim & 3) == 1) drawfur(P_WC,xdir);
			if ((anim & 3) == 2) drawfur(P_STANDING,xdir);
			if ((anim & 3) == 3) drawfur(P_WC+2,xdir);
			if (!xdir) {
				if (xpos) {
					xpos-=2;
				} else {
					xdir = 1;
				}
			} else {
				if (xpos < 51) {
					xpos+=2;
				} else {
					xdir = 0;
				}
			}
		}

		if (((state == SLEEP) || (state == FA)) && (b > 100)) {
			lcdxy(22,2);
			txlcd(0);
			txlcd(0);
			txlcd(0);
			txlcd(0);
			txlcd(0);
			txlcd(0);
			txlcd(0);
			txlcd(0);
			txlcd(0);
			drawfur(P_STANDING,xdir);
			state = AWAKE;
			valbeep();
			_delay_ms(80);
			valbeep();
			_delay_ms(80);
		}
		if ((state == AWAKE) && (b < 80)) {
			drawfur(P_FOURS,xdir);
			fatimer = 10;
			state = FA;
			valbeep();
		}
		if (state == FA) {
			if (fatimer)
				fatimer--;
			else {
				drawfur(P_SLEEPING,xdir);
				selbeep();
				state = SLEEP;
			}
		}

		anim++;
		_delay_ms(200);
	}

    return 0;
}
