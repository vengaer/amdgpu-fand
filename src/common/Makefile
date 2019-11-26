SRC_BASE	:= $(BASE_DIR)/src
SRC_DIR    	:= $(SRC_BASE)/$(COMMON)

CFLAGS		:= $(G_CFLAGS)
LIB	        := $(G_LIB)

SRC        	:= $(shell find $(SRC_DIR) -mindepth 1 -type f -name "*.$(SRC_EXT)")
OBJ        	:= $(patsubst $(SRC_BASE)/%, $(BUILD_DIR)/%, $(SRC:.$(SRC_EXT)=.$(OBJ_EXT)))

all: $(OBJ)

$(BUILD_DIR)/%.$(OBJ_EXT): $(SRC_BASE)/%.$(SRC_EXT)
	$(info Compiling $(patsubst $(BUILD_DIR)/%, %, $@))
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INC) -MD -MP -c -o $@ $<

.PHONY: clean 

clean:
	$(info Cleaning $(COMMON))
	@rm -f $(TARGET_PATH); rm -rf $(BUILD_DIR)/$(COMMON)

-include $(OBJ:.o=.d)
