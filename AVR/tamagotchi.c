//Kemochi - furrtek 2015

//IR packet:
//B1 sccccccc 00llllll d... kkkkkkkk
//s = source (0=programmer, 1=other kemochi)
//c = command
//l = length
//k = checksum (start=0xAA)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

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

uint8_t volatile buf[32];

void txlcd(uint8_t b) {
	uint8_t c;

	PORTA &= ~_BV(PA2);
	_delay_us(5);
	for (c=0;c<8;c++) {
		if ((b<<c)&0x80)
			PORTA |= _BV(PA5);
		else
			PORTA &= ~_BV(PA5);
		_delay_us(5);
		PORTA |= _BV(PA4);
		_delay_us(5);
		PORTA &= ~_BV(PA4);
	}
	_delay_us(5);
	PORTA |= _BV(PA2);
	_delay_us(5);
}

void clrlcd() {
	uint8_t c;
	PORTA |= 0b00001000;
	for (c=0;c<252;c++) {
		txlcd(0);
		txlcd(0);
	}
}

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
						for (c=0;c<32;c++) {
							PORTB ^= 1;
							_delay_us(200);
						}
						PORTB = 0;

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

#define SDA PA7
#define SCL PA4

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
	_delay_us(5);
  	i2c_io_set_sda(0);
	_delay_us(5);
  	i2c_io_set_scl(0);
	_delay_us(20);
}

void i2c_stop() {
	_delay_us(5);
  	i2c_io_set_scl(0);
	_delay_us(5);
  	i2c_io_set_sda(0);
	_delay_us(5);
  	i2c_io_set_scl(1);
	_delay_us(5);
  	i2c_io_set_sda(1);
}

uint8_t i2c_write(uint8_t byte) {
	uint8_t ack, bit;

	for (bit=0;bit<8;bit++) {
		if (((byte<<bit) & 0x80) == 0x80)
			i2c_io_set_sda(1);
		else
			i2c_io_set_sda(0);
		_delay_us(3);
  		i2c_io_set_scl(1);
		_delay_us(3);
  		i2c_io_set_scl(0);
	}
	i2c_io_set_sda(1);
	_delay_us(10);

  	i2c_io_set_scl(1);
	_delay_us(5);
	ack = (PINA & SDA)>>SDA;
	_delay_us(5);
  	i2c_io_set_scl(0);
	_delay_us(5);
	return ack;
}

uint8_t i2c_read() {
	uint8_t byte = 0x00,c, bit;
	for (bit=0;bit<8;bit++) {
  		i2c_io_set_sda(1);
    	i2c_io_set_scl(0);
		_delay_us(5);
    	i2c_io_set_scl(1);
		_delay_us(30);
    	c = PINA & _BV(SDA);
		byte <<= 1;
    	if (c) byte |= 1;
		_delay_us(30);
    	i2c_io_set_scl(0);
	}
	return byte;
}

void i2c_readack() {
   	i2c_io_set_scl(0);
	_delay_us(5);
  	i2c_io_set_sda(0);
	_delay_us(5);
   	i2c_io_set_scl(1);
	_delay_us(5);
   	i2c_io_set_scl(0);
	_delay_us(5);
  	i2c_io_set_sda(1);
	_delay_us(5);
}

void i2c_noreadack() {
   	i2c_io_set_scl(0);
	_delay_us(5);
  	i2c_io_set_sda(1);
	_delay_us(5);
   	i2c_io_set_scl(1);
	_delay_us(5);
   	i2c_io_set_scl(0);
	_delay_us(10);
}

uint8_t exee_read_byte(uint16_t addr) {
	uint8_t v;
	i2c_start();
	i2c_write(0xA0);
	i2c_write(addr>>8);
	i2c_write(addr & 0xFF);
	_delay_ms(1);
	i2c_start();
	i2c_write(0xA1);
	v = i2c_read();
	i2c_noreadack();
	return v;
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
				txlcd(exee_read_byte(cv+tc));
		}
		txlcd(0);
		txt++;
	}
}

int main(void) {
	uint8_t c,b,bc;

	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = 0x00;

	DDRA = 0b01111100;
	DDRB = 0b00000001;

	MCUCR = 0b00000010;
	GIMSK = 0b01000000;

	TCCR1A = 0b00000000;
	TCCR1B = 0b00000001;
	TIMSK1 = 0b00000001;

	//Reset LCD
	PORTA = 0b11000100;
	_delay_ms(50);
	PORTA = 0b10000100;
	_delay_ms(10);
	PORTA = 0b11000100;
	_delay_ms(50);

	txlcd(0x21);
	txlcd(0xAC);
	txlcd(0x14);
	txlcd(0x20);
	txlcd(0x0C);

	clrlcd();

	ADMUX = 0b10000000;
	ADCSRA = 0b10000100;
	ADCSRB = 0b00010000;
	DIDR0 = 0b00000001;

	PORTA &= ~0b00001000;
	txlcd(0x40);
	txlcd(0x80);
	PORTA |= 0b00001000;

	for (c=0;c<20;c++) {
		for (b=0;b<26;b++) {
			if ((c*26)+b <= 504) txlcd(exee_read_byte((c*64)+b));
		}
	}

	//lcdtxt("0123456789");

	sei();

	for(;;) {
		b = get_adc();
		//sprintf(mbuf,"ADC0:%u   ",b);
		//lcdtxt(mbuf);
		_delay_ms(200);

		if (((state == SLEEP) || (state == FA)) && (b > 100)) {
			PORTA &= ~0b00001000;
			txlcd(0x41);
			txlcd(0x80);
			PORTA |= 0b00001000;
			lcdtxt("AWAKE   ");
			state = AWAKE;
		}
		if ((state == AWAKE) && (b < 80)) {
			PORTA &= ~0b00001000;
			txlcd(0x41);
			txlcd(0x80);
			PORTA |= 0b00001000;
			lcdtxt("SLEEP ?");
			fatimer = 10;
			state = FA;
		}
		if (state == FA) {
			if (fatimer)
				fatimer--;
			else {
				PORTA &= ~0b00001000;
				txlcd(0x41);
				txlcd(0x80);
				PORTA |= 0b00001000;
				lcdtxt("SLEEPING");
				state = SLEEP;
			}
		}
	}

    return 0;
}
