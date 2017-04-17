#include "main.h"
#include "progmem.h"

void load_struct(uint8_t * dest, uint8_t * src, uint8_t size) {
	uint8_t b;

	for (b = 0; b < size; b++)
		*dest++ = pgm_read_byte(src++);
}
