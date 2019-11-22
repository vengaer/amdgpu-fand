CC           ?= gcc

TARGET       := amdgpu-fanctl

SRC_DIR      := src
INC_DIRS     := $(shell find $(SRC_DIR) -mindepth 1 -type d)
BUILD_DIR    := build
SRC_EXT      := c
OBJ_EXT      := o
CFLAGS       := -std=c11 -Wall -Wextra -pedantic -Wshadow -Wunknown-pragmas -O3 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE
LIB          := -lm -lpthread
INC          := $(shell [ -z "${INC_DIRS}" ] || echo "${INC_DIRS}" | sed -E 's/( |^)([^ ]*)/-I \2 /g')

SRC          := $(shell find $(SRC_DIR) -mindepth 1 -type f -name *.$(SRC_EXT))
OBJ          := $(patsubst $(SRC_DIR)/%, $(BUILD_DIR)/%, $(SRC:.$(SRC_EXT)=.$(OBJ_EXT)))

CONFIG       := $(TARGET).conf
SERVICE      := $(TARGET).service
INSTALL_DIR  := /usr/local/bin
CONFIG_DIR   := /etc
SERVICE_DIR  := /etc/systemd/system


all: $(TARGET)

$(TARGET): $(OBJ)
	$(info Linking $@)
	@$(CC) -o $@ $^ $(LIB)

$(BUILD_DIR)/%.$(OBJ_EXT): $(SRC_DIR)/%.$(SRC_EXT) | dirs
	$(info Compiling $@)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INC) -MD -MP -c -o $@ $<

.PHONY: run clean install dirs

run: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET); rm -rf $(BUILD_DIR)

install: $(TARGET)
	$(info Installing to $(INSTALL_DIR)/$(TARGET))
	@install $(TARGET) $(INSTALL_DIR)/$(TARGET)
	$(info Creating default config in $(CONFIG_DIR)/$(CONFIG))
	@install -m644 $(CONFIG) $(CONFIG_DIR)/$(CONFIG)
	$(info Adding systemd service $(SERVICE_DIR)/$(SERVICE))
	@install -m644 $(SERVICE) $(SERVICE_DIR)/$(SERVICE)

dirs:
	@mkdir -p $(BUILD_DIR)

-include $(OBJ:.o=.d)
