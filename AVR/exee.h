#include "main.h"

#define EE_WRITE 0xA0
#define EE_READ 0xA1


#define EEP_ICONS_A 0
#define EEP_ICONS_B 1

#define EEP_PINFO_A 8
#define EEP_PINFO_B 9

uint8_t exee_buf[64];
uint8_t exee_last_page;

typedef struct {
	char name[12];
	char species[12];
	char country[12];
	char speaksA[14];
	char speaksB[14];
} player_info_A;

typedef struct {
	char likesA[12];
	char likesB[12];
	char likesC[12];
	char birthyear[4];
	char padding[24];
} player_info_B;

// EEPROM layout:

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

//Page F0: Sound effects

void exee_set_address(uint16_t addr);
uint8_t exee_read_byte(uint16_t addr);
void exee_read_page(uint16_t addr);
