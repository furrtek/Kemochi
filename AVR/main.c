// Kemochi - furrtek 2015

// IR packet:
// 10110001 sccccccc 000lllll d... kkkkkkkk
// s = source (0=programmer, 1=other kemochi)
// c = command
// l = length
// k = checksum (start=0xAA)

// Todo: Use an LCD line with LCD /CE not asserted for buzzer, not a button line !

#define P_STANDING	0x10
#define P_FOURS		0x12
#define P_SLEEPING	0x14
#define P_WC_A		0x16
#define P_WC_B		0x18
#define P_EAT		0x1A

#include "main.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/pgmspace.h>

#include "i2c.h"
#include "exee.h"
#include "sound.h"
#include "lcd.h"
#include "ir.h"

#define AWAKE 0
#define SLEEP 1
#define FA 2

uint8_t get_adc() {
	uint8_t value;

	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)); 
	value = ADCH;	// Discard

	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	return ADCH;
}

uint8_t volatile ir_rx_dibits, ir_rx_sr, ir_rx_state, ir_rx_index, state, fatimer;
uint16_t volatile stat_sleepy = 0;
uint16_t volatile xpos = 26, xdir;
uint8_t volatile icon_sel = 0;

typedef struct {
	uint8_t x;
	uint8_t page;
	uint8_t start;
} icon_def_t;

const icon_def_t PROGMEM icon_defs[10] = {
	{ 0+7, EEP_ICONS_A, 0 },
	{ 15+7, EEP_ICONS_A, 9 },
	{ 30+7, EEP_ICONS_A, 18 },
	{ 45+7, EEP_ICONS_A, 27 },
	{ 60+7, EEP_ICONS_A, 36 },
	{ 0+7, EEP_ICONS_A, 45 },
	{ 15+7, EEP_ICONS_A, 54 },
	{ 30+7, EEP_ICONS_B, 0 },
	{ 45+7, EEP_ICONS_B, 9 },
	{ 60+7, EEP_ICONS_B, 18 }
};

const uint8_t PROGMEM walk_cycle[4] = {
	P_STANDING, P_WC_A, P_STANDING, P_WC_B
};

void load_struct(uint8_t * dest, uint8_t * src, uint8_t size) {
	uint8_t b;

	for (b = 0; b < size; b++)
		*dest++ = pgm_read_byte(src++);
}

void refresh_icons() {
	icon_def_t icon_def;
	uint8_t i, page;

	exee_last_page = 0xFF;

	// This is SLOW !
	for (i = 0; i < 10; i++) {
		load_struct((uint8_t*)&icon_def, (uint8_t*)&icon_defs[i], sizeof(icon_def_t));
		page = icon_def.page;
		if (page != exee_last_page)
			exee_read_page(page);
		lcd_draw(icon_def.x, 5 - (i >> 1), icon_def.start, 9, (icon_sel == i) ? 0xFF : 0x00, 0);
	}
}

// Button press
ISR(PCINT0_vect) {

	_delay_ms(5);	// Debounce

	// Left (PCINT1)
	if (!(PINA & _BV(BTN_C))) {
		valbeep();
	}

	// Select (PCINT8)
	if (!(PINB & _BV(BTN_A))) {
		if (icon_sel == 9)
			icon_sel = 0;
		else
			icon_sel++;
		refresh_icons();
		selbeep();
	}

	// Right (PCINT9)
	if (!(PINB & _BV(BTN_B))) {
		valbeep();
	}

	_delay_ms(15);	// Debounce

	//PCMSK1 = 0b00000011;
	//GIFR = 0;
}

ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));

// IR timeout
ISR(TIM1_OVF_vect) {
	if (ir_rx_index)
		ir_rx_state = IR_RX_DECODE;

	TCCR1B = 0b00000000;		// Stop timer
}

// IR receive
ISR(INT0_vect) {
	uint16_t t;

	t = TCNT1;
	TCNT1 = 0;

	if (ir_rx_state == IR_RX_IDLE) {
		ir_rx_index = 0;			// Init
		ir_rx_dibits = 0;
		ir_rx_state = IR_RX_IDLE;

		TCCR1B = 0b00000001;		// Start timer
		ir_rx_state = IR_RX_RECV;
	} else if (ir_rx_state == IR_RX_RECV) {
		// New dibit

		ir_rx_sr >>= 2;

		if ((t >= 800) && (t < 1600))
			ir_rx_sr |= 0x00;
		else if ((t >= 1600) && (t < 2400))
			ir_rx_sr |= 0x40;
		else if ((t >= 2400) && (t < 3200))
			ir_rx_sr |= 0x80;
		else	// if ((v >= 3200) && (v < 4000))
			ir_rx_sr |= 0xC0;

		if (ir_rx_dibits == 3) {
			ir_rx_buf[ir_rx_index & 0x1F] = ir_rx_sr;
			ir_rx_index++;
			ir_rx_dibits = 0;
		} else {
			ir_rx_dibits++;
		}
	}
}

int main(void) {
	uint8_t c, anim = 0, light, frame;

	// Disable watchdog
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = 0x00;
	
	DDRA = _BV(I2C_SDA) | _BV(LCD_RST) | _BV(LCD_DIN) | _BV(LCD_CLK) | _BV(LCD_DC) | _BV(LCD_CE);
	DDRB = 0b00000000;
	PORTA = _BV(BTN_C);		// Pull-ups
	PORTB = _BV(BTN_B) | _BV(BTN_A);

	MCUCR = 0b00000010;		// INT0 falling edge
	GIMSK = 0b01110000;		// All pin interrupts enabled
	PCMSK1 = 0b00000011;	// Buttons PCINT masks
	PCMSK0 = 0b00000010;

	TCCR1A = 0b00000000;	// Normal count @ 1MHz
	//TCCR1B = 0b00000001;
	TIMSK1 = 0b00000001;	// Overflow interrupt

	PORTA = 0b11000110;		// Reset LCD
	_delay_ms(50);
	PORTA = 0b10000110;
	_delay_ms(10);
	PORTA = 0b11000110;
	_delay_ms(50);

	lcd_tx(0x21);		// No power-down, horizontal addressing, extended instructions
	lcd_tx(0xAE);		// Vop = 0x2E
	lcd_tx(0x13);		// Bias system n = 3
	lcd_tx(0x20);		// Basic instruction set
	lcd_tx(0x0C);		// Normal display mode

	lcd_clear();

	ADMUX = 0b10000000;		// 1.1V reference
	ADCSRA = 0b10000100;	// ADC enable
	ADCSRB = 0b00010000;	// ADLAR
	DIDR0 = 0b00000001;		// Digital input off on PA0

	refresh_icons();

	//lcdxy(20,3);
	//lcdtxt("0123456789");

	drawfur(P_STANDING, 0);

	sei();

	for (;;) {

		if (ir_rx_state == IR_RX_DECODE) {
			ir_decode();
			ir_rx_state = IR_RX_IDLE;
		}

		light = get_adc();

		//sprintf(mbuf,"ADC0:%u   ",b);
		//lcdtxt(mbuf);

		//Fall asleep: return to center of screen (xpos=26)
		//Then all4s
		//Then sleeping
		//Don't fuck around with Zz icon's position

		if (state == SLEEP) {
			// Animate "Zz"
			if (anim & 1) {
				frame = 18 + ((anim & 2) >> 1) * 9;
				exee_read_page(1);
				if (xpos >= 10)
					lcd_draw(xpos - 10, 2, frame, 9, 0, 0);
				else
					lcd_draw(xpos + 32 + 1, 2, frame, 9, 0, 0);
			}
			// Decrease sleepiness
			if (stat_sleepy)
				stat_sleepy--;
		}

		if (state == AWAKE) {
			frame = anim & 2;

			drawfur(pgm_read_byte(&walk_cycle[frame]), xdir);

			if (!xdir) {
				if (xpos >= 2) {
					xpos -= 2;
				} else {
					xdir = 1;
				}
			} else {
				if (xpos < 51) {
					xpos += 2;
				} else {
					xdir = 0;
				}
			}
		}

		if (((state == SLEEP) || (state == FA)) && (light > 100)) {
			lcd_xy(22, 2);
			lcd_tx(0);
			lcd_tx(0);
			lcd_tx(0);
			lcd_tx(0);
			lcd_tx(0);
			lcd_tx(0);
			lcd_tx(0);
			lcd_tx(0);
			lcd_tx(0);
			drawfur(P_STANDING, xdir);
			state = AWAKE;
			valbeep();
			_delay_ms(80);
			valbeep();
			_delay_ms(80);
		}
		if ((state == AWAKE) && (light < 80)) {
			drawfur(P_FOURS, xdir);
			fatimer = 10;
			state = FA;
			valbeep();
		}
		if (state == FA) {
			if (fatimer)
				fatimer--;
			else {
				drawfur(P_SLEEPING, xdir);
				selbeep();
				state = SLEEP;
			}
		}

		anim++;
		_delay_ms(200);
	}

    return 0;
}
