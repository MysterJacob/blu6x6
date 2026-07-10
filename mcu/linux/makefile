SHELL := bash

CC ?= gcc
AR ?= ar

TARGET_NAME ?= linux_build

LD_FLAGS :=
CC_EXTRA_FLAGS ?=
CC_FLAGS := -Wall -Wextra -Os -pedantic --std=c2x $(CC_EXTRA_FLAGS)

PLATFORM_DIR := .
COMMON_DIR := ../common

SRC_DIRS := $(PLATFORM_DIR)/src $(COMMON_DIR)/src
INC_DIRS := $(PLATFORM_DIR)/include $(COMMON_DIR)/include

BIN_DIR := $(PLATFORM_DIR)/build
BUILD_DIR := $(BIN_DIR)/bin
OBJ_DIR := $(BIN_DIR)/obj
TEST_OBJ_DIR := $(BIN_DIR)/tests
TEST_SRC_DIR := $(PLATFORM_DIR)/test

PROJECT_TARGET := $(BUILD_DIR)/$(TARGET_NAME)
TEST_TARGET := $(BUILD_DIR)/test

vpath %.c $(SRC_DIRS)
vpath %.h $(INC_DIRS)

INCLUDES := $(addprefix -I,$(INC_DIRS))

PLATFORM_SOURCES := $(notdir $(wildcard $(PLATFORM_DIR)/src/*.c))
COMMON_SOURCES := $(notdir $(wildcard $(COMMON_DIR)/src/*.c))
SOURCES := $(PLATFORM_SOURCES) $(COMMON_SOURCES)

OBJ_FILES := $(addprefix $(OBJ_DIR)/,$(SOURCES:.c=.o))

TEST_SOURCES := $(notdir $(wildcard $(TEST_SRC_DIR)/*.c))
TEST_OBJ_FILES := $(addprefix $(TEST_OBJ_DIR)/,$(TEST_SOURCES:.c=.o))

default: $(PROJECT_TARGET)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR) $(BUILD_DIR) $(OBJ_DIR) $(TEST_OBJ_DIR)

$(PROJECT_TARGET): $(BIN_DIR) $(OBJ_FILES)
	$(CC) $(CC_FLAGS) $(OBJ_FILES) $(LD_FLAGS) -o $@

$(OBJ_DIR)/%.o: %.c | $(BIN_DIR)
	$(CC) $(CC_FLAGS) $(INCLUDES) -c $< -o $@

$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CC_FLAGS) $(INCLUDES) -c $< -o $@

$(TEST_TARGET): $(BIN_DIR) $(OBJ_FILES) $(TEST_OBJ_FILES)
	$(CC) $(CC_FLAGS) $(INCLUDES) \
		$(TEST_OBJ_FILES) \
		$(filter-out $(OBJ_DIR)/main.o,$(OBJ_FILES)) \
		$(LD_FLAGS) \
		-o $@

run: $(PROJECT_TARGET)
	./$(PROJECT_TARGET)

test: clean $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf $(BIN_DIR)

print:
	@echo PLATFORM_SOURCES=$(PLATFORM_SOURCES)
	@echo COMMON_SOURCES=$(COMMON_SOURCES)
	@echo OBJ_FILES=$(OBJ_FILES)

.PHONY: default clean test print run
