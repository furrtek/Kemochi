// Kemochi fw 0.1
// furrtek 2015~2017
// ATTiny44 1MHz RC

// Todo: Use an LCD line with LCD /CE not asserted for buzzer, not a button line !

// 79.2%
// 78.1%
// 74.8%
// 73.8%
// 73.7%

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

#include "i2c.h"
#include "exee.h"
#include "sound.h"
#include "lcd.h"
#include "progmem.h"
#include "icon.h"
#include "ir.h"

#define AWAKE 0
#define SLEEP 1
#define FALLING_ASLEEP 2
#define WAKING_UP 3

uint8_t get_adc() {
	uint8_t value;

	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)); 
	value = ADCH;	// Discard

	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	return ADCH;
}

uint8_t volatile ir_rx_dibits, ir_rx_sr, ir_rx_state, ir_rx_index, state, anim_timer, tick = 0;
uint16_t volatile stat_sleepy = 0;
uint16_t volatile xpos = 26, xdir;
uint8_t volatile icon_sel = 0, prev_icon, need_refresh = 0;

const uint8_t PROGMEM walk_cycle[4] = {
	P_STANDING, P_WC_A, P_STANDING, P_WC_B
};

// Button press
ISR(PCINT0_vect) {

	_delay_ms(10);	// Debounce

	// Left (PCINT8)
	if (!(PINB & _BV(BTN_A))) {
		prev_icon = icon_sel;
		if (icon_sel == 0)
			icon_sel = 9;
		else
			icon_sel--;
		need_refresh = 1;
		valbeep();
	}

	// Right (PCINT1)
	if (!(PINA & _BV(BTN_C))) {
		prev_icon = icon_sel;
		if (icon_sel == 9)
			icon_sel = 0;
		else
			icon_sel++;
		need_refresh = 1;
		valbeep();
	}

	// Select (PCINT9)
	if (!(PINB & _BV(BTN_B))) {
		selbeep();
	}

	_delay_ms(10);	// Debounce

	PCMSK1 = 0b00000011;
}

ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));

// Animation tick
ISR(TIM0_OVF_vect) {
	tick = 1;
}

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

void wake_up() {
	lcd_clear(1, 4);
	draw_fur(P_FOURS, xdir);
	anim_timer = 10;
	state = WAKING_UP;
	valbeep();
}

int main(void) {
	uint8_t anim = 0, light, frame;

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
	
	TCCR0A = 0b00000000;	// Normal count @ 1MHz / 256
	TCCR0B = 0b00000100;
	TIMSK0 = 0b00000001;	// Overflow interrupt

	_delay_ms(10);
	PORTA &= ~_BV(LCD_CE);
	PORTA = 0b11000110;		// Reset LCD
	_delay_ms(10);
	PORTA = 0b10000110;
	_delay_ms(50);
	PORTA = 0b11000110;
	_delay_ms(10);
	PORTA |= _BV(LCD_CE);

	PORTA &= ~_BV(LCD_DC);
	lcd_write(0x21);		// No power-down, horizontal addressing, extended instructions
	lcd_write(0x14);		// Bias system n = 3
	lcd_write(0xB5);		// Vop
	lcd_write(0x20);		// Basic instruction set
	lcd_write(0x0C);		// Normal display mode

	lcd_clear(0, 6);

	ADMUX = 0b10000000;		// 1.1V reference
	ADCSRA = 0b10000100;	// ADC enable
	ADCSRB = 0b00010000;	// ADLAR
	DIDR0 = 0b00000001;		// Digital input off on PA0

	state = AWAKE;

	draw_icons();

	//lcdxy(20,3);
	//lcdtxt("0123456789");

	draw_fur(P_STANDING, 0);

	sei();

	for (;;) {

		tick = 0;

		while (!tick) {
			if (ir_rx_state == IR_RX_DECODE) {
				ir_decode();
				ir_rx_state = IR_RX_IDLE;
			}

			if (need_refresh) {
				refresh_icon();
				need_refresh = 0;
			}
		}

		light = get_adc();

		// Sleepy time u.u
		if (state == SLEEP) {
			// Animate "Zz"
			if (anim & 1) {
				frame = 27 + ((anim & 2) >> 1) * 9;
				exee_read_page(1);
				if (xpos >= 10)
					lcd_draw(xpos - 10, 2, frame, 9, 0, 0);
				else
					lcd_draw(xpos + 33, 2, frame, 9, 0, 0);
			}
			if (stat_sleepy)
				stat_sleepy--;	// Decrease sleepiness
			else
				wake_up();		// Wake up if not tired anymore
		}

		// Awake, walking
		if ((state == AWAKE) && (!(anim & 3))) {
			frame = (anim >> 2) & 3;

			draw_fur(pgm_read_byte(&walk_cycle[frame]), xdir);

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

			if (stat_sleepy != 255)
				stat_sleepy++;	// Increase sleepiness
		}

		// Wake up because of light if not too sleepy
		if (((state == SLEEP) || (state == FALLING_ASLEEP)) && (light > 90)) {
			if (stat_sleepy < 64)
				wake_up();
		}

		// Fall asleep because of dark if sleepy enough
		if ((state == AWAKE) && (light < 80) && (stat_sleepy > 128)) {
			draw_fur(P_FOURS, xdir);
			anim_timer = 10;
			state = FALLING_ASLEEP;
			valbeep();
		}

		if (anim_timer)
			anim_timer--;
		else {
			if (state == FALLING_ASLEEP) {
				draw_fur(P_SLEEPING, xdir);
				selbeep();
				state = SLEEP;
			} else if (state == WAKING_UP) {
				draw_fur(P_STANDING, xdir);
				selbeep();
				_delay_ms(80);
				valbeep();
				_delay_ms(80);
				state = AWAKE;
			}
		}


		anim++;
	}

    return 0;
}
