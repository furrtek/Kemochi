#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>

#define IR_LED	PA0

#define LCD_CE	PA2
#define LCD_DC	PA3
#define LCD_CLK	PA4
#define LCD_DIN	PA5
#define LCD_RST	PA6

#define BTN_A	PB0
#define BTN_B	PB1
#define BTN_C	PA1

#define I2C_SDA	PA7
#define I2C_SCL	PA4

// IR stuff

#define IR_RX_IDLE 0
#define IR_RX_RECV 1
#define IR_RX_DECODE 2

uint8_t volatile ir_rx_buf[32];
