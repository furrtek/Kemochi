#include "main.h"

void lcd_write(uint8_t b);
void lcd_clear(uint8_t start, uint8_t count);
void lcd_txt(char * txt, uint8_t maxlen);
void lcd_xy(uint8_t x, uint8_t y);
void lcd_draw(uint8_t x, uint8_t y, uint8_t s, uint8_t e, uint8_t inv, uint8_t flip);
void draw_fur(uint8_t frame, uint8_t flip);
