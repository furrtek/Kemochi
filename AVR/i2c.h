#include "main.h"

void i2c_io_set_sda(uint8_t hi);
void i2c_io_set_scl(uint8_t hi);
void i2c_start();
void i2c_stop();
uint8_t i2c_write(uint8_t byte);
uint8_t i2c_read();
void i2c_readack();
void i2c_noreadack();
