#include "main.h"

#define IR_MAGIC 0xB1

#define IR_CMD_PING 1

#define IR_CMD_FLASHPROG 8
#define IR_CMD_EEPROG 9

#define IR_CMD_SPECID 16

void ir_decode();
