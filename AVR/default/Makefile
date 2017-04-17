###############################################################################
# Makefile for the project tamagochi
###############################################################################

## General Flags
PROJECT = tamagochi
MCU = attiny44
TARGET = kemochi.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99         -DF_CPU=1000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=kemochi.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = lcd.o exee.o main.o i2c.o sound.o ir.o icon.o progmem.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) kemochi.hex kemochi.eep kemochi.lss size

## Compile
lcd.o: ../lcd.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

exee.o: ../exee.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

main.o: ../main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

i2c.o: ../i2c.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

sound.o: ../sound.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ir.o: ../ir.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

icon.o: ../icon.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

progmem.o: ../progmem.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) kemochi.elf dep/* kemochi.hex kemochi.eep kemochi.lss kemochi.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

