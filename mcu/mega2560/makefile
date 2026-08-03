SHELL := bash

TARGET_NAME ?= mega2560_build

MCU ?= atmega2560
F_CPU ?= 16000000UL

CC := avr-gcc
AR := avr-ar
OBJCOPY := avr-objcopy
SIZE := avr-size
AVRDUDE ?= avrdude

PROGRAMMER ?= wiring
PORT ?= /dev/ttyACM0
BAUD ?= 115200

PLATFORM_DIR := .
COMMON_DIR := ../common

SRC_DIRS := $(PLATFORM_DIR)/src $(COMMON_DIR)/src
INC_DIRS := $(PLATFORM_DIR)/include $(COMMON_DIR)/include

BIN_DIR := $(PLATFORM_DIR)/build
BUILD_DIR := $(BIN_DIR)/bin
OBJ_DIR := $(BIN_DIR)/obj
LIB_DIR := $(PLATFORM_DIR)/lib

PROJECT_ELF := $(BUILD_DIR)/$(TARGET_NAME).elf
PROJECT_HEX := $(BUILD_DIR)/$(TARGET_NAME).hex
PROJECT_LSS := $(BUILD_DIR)/$(TARGET_NAME).lss
PROJECT_MAP := $(BUILD_DIR)/$(TARGET_NAME).map

vpath %.c $(SRC_DIRS)
vpath %.h $(INC_DIRS)

INCLUDES := $(addprefix -I,$(INC_DIRS))

PLATFORM_SOURCES := $(notdir $(wildcard $(PLATFORM_DIR)/src/*.c))
COMMON_SOURCES := $(notdir $(wildcard $(COMMON_DIR)/src/*.c))
SOURCES := $(PLATFORM_SOURCES) $(COMMON_SOURCES)

OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(SOURCES:.c=.o))

COMMON_WARN := -Wall -Wextra -Os
CSTD := -std=gnu1x
MCU_FLAGS := -mmcu=$(MCU) -DF_CPU=$(F_CPU)
SECTION_FLAGS := -ffunction-sections -fdata-sections
DEP_FLAGS := -MMD -MP
CC_EXTRA_FLAGS ?=
CFLAGS := $(COMMON_WARN) $(CSTD) $(MCU_FLAGS) $(SECTION_FLAGS) $(DEP_FLAGS) $(CC_EXTRA_FLAGS)

LD_EXTRA_FLAGS ?=
LDFLAGS := -mmcu=$(MCU) -Wl,--gc-sections -Wl,-Map=$(PROJECT_MAP) $(LD_EXTRA_FLAGS) -Wl,-u,vfprintf -lprintf_flt -lm

AVRDUDE_FLAGS := -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b $(BAUD)

default: all
all: $(PROJECT_HEX)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR) $(BUILD_DIR) $(OBJ_DIR)

$(PROJECT_ELF): $(BIN_DIR) $(OBJ_FILES)
	$(CC) $(OBJ_FILES)  $(LDFLAGS) -o $@

$(PROJECT_HEX): $(PROJECT_ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) --mcu=$(MCU) --format=avr $(PROJECT_ELF)

$(OBJ_DIR)/%.o: %.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

flash: $(PROJECT_HEX)
	$(AVRDUDE) $(AVRDUDE_FLAGS) -D -U flash:w:$(PROJECT_HEX):i

verify: $(PROJECT_HEX)
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U flash:v:$(PROJECT_HEX):i

disasm: $(PROJECT_ELF)
	avr-objdump -d -S $(PROJECT_ELF) > $(PROJECT_LSS)

size: $(PROJECT_ELF)
	$(SIZE) --mcu=$(MCU) --format=avr $(PROJECT_ELF)

clean:
	rm -rf $(BIN_DIR)

print:
	@echo SOURCES=$(SOURCES)
	@echo OBJ_FILES=$(OBJ_FILES)
	@echo PROJECT_ELF=$(PROJECT_ELF)
	@echo PROJECT_HEX=$(PROJECT_HEX)

-include $(OBJ_FILES:.o=.d)

.PHONY: default all flash verify disasm size clean print
