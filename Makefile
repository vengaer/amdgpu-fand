CC           ?= gcc

TARGET       := amdgpu-fan

SRC_DIR      := src
INC_DIRS     := $(shell find $(SRC_DIR) -mindepth 1 -type d)
BUILD_DIR    := build
SRC_EXT      := c
OBJ_EXT      := o
CFLAGS       := -std=c11 -Wall -Wextra -pedantic -Wshadow -Wunknown-pragmas
LIB          := 
INC          := $(shell [ -z "${INC_DIRS}" ] || echo "${INC_DIRS}" | sed -E 's/( |^)([^ ]*)/-I \2 /g')

SRC          := $(shell find $(SRC_DIR) -mindepth 1 -type f -name *.$(SRC_EXT))
OBJ          := $(patsubst $(SRC_DIR)/%, $(BUILD_DIR)/%, $(SRC:.$(SRC_EXT)=.$(OBJ_EXT)))


all: $(TARGET)

$(TARGET): $(OBJ)
	$(info Linking $@)
	@$(CC) -o $@ $^ $(LIB)

$(BUILD_DIR)/%.$(OBJ_EXT): $(SRC_DIR)/%.$(SRC_EXT) | dirs
	$(info Compiling $@)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INC) -MD -MP -c -o $@ $<

.PHONY: run clean dirs

run: $(TARGET)
	@./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET); rm -rf $(BUILD_DIR)

dirs:
	@mkdir -p $(BUILD_DIR)

-include $(OBJ:.o=.d)
