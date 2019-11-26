TARGET     	:= amdgpu-fanctl
TARGET_PATH := $(BASE_DIR)/$(TARGET)

SRC_BASE	:= $(BASE_DIR)/src
SRC_DIR    	:= $(SRC_BASE)/$(TARGET)
INC_DIRS   	:= $(shell find $(SRC_DIR) $(SRC_BASE)/$(COMMON) -type d)
INC	        := $(foreach d, $(INC_DIRS), -I$d)

CFLAGS		:= $(G_CFLAGS)
LIB	        := $(G_LIB)

SRC        	:= $(shell find $(SRC_DIR) -mindepth 1 -type f -name "*.$(SRC_EXT)")
OBJ        	:= $(patsubst $(SRC_BASE)/%, $(BUILD_DIR)/%, $(SRC:.$(SRC_EXT)=.$(OBJ_EXT))) \
			   $(wildcard $(BUILD_DIR)/$(COMMON)/*.$(OBJ_EXT))

all: $(TARGET_PATH)

$(TARGET_PATH): CFLAGS += $(INC)
$(TARGET_PATH): $(OBJ)
	$(info Linking $(TARGET))
	@$(CC) -o $@ $^ $(LIB)

$(BUILD_DIR)/%.$(OBJ_EXT): $(SRC_BASE)/%.$(SRC_EXT)
	$(info Compiling $(patsubst $(BUILD_DIR)/%, %, $@))
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INC) -MD -MP -c -o $@ $<

.PHONY: clean info

clean:
	$(info Cleaning $(TARGET))
	@rm -f $(TARGET_PATH); rm -rf $(BUILD_DIR)/$(TARGET)

-include $(OBJ:.o=.d)
