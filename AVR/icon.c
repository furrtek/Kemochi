#include "icon.h"
#include "lcd.h"
#include "progmem.h"
#include "exee.h"

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

void draw_icon(uint8_t n, uint8_t invert) {
	icon_def_t icon_def;
	uint8_t page;

	exee_last_page = 0xFF;

	load_struct((uint8_t*)&icon_def, (uint8_t*)&icon_defs[n], sizeof(icon_def_t));
	page = icon_def.page;
	if (page != exee_last_page)
		exee_read_page(page);
	lcd_draw(icon_def.x, (n < 5) ? 0 : 5, icon_def.start, 9, invert, 0);
}

void draw_icons() {
	uint8_t i;

	// This is SLOW !
	for (i = 0; i < 10; i++)
		draw_icon(i, (icon_sel == i) ? 0xFF : 0x00);
}

void refresh_icon() {
	// Redraw previous icon
	draw_icon(prev_icon, 0x00);

	// Redraw current icon
	draw_icon(icon_sel, 0xFF);
}
