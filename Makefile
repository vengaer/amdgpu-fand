CC           ?= gcc

BASE_DIR 	 := $(shell pwd)
COMMON	     := common

DAEMON		 := amdgpu-fand
CONTROLLER	 := amdgpu-fanctl

SRC_DIR		 := src
BUILD_DIR    := $(BASE_DIR)/build
SRC_EXT      := c
OBJ_EXT      := o
CFLAGS       := -std=c11 -Wall -Wextra -pedantic -Wshadow -Wunknown-pragmas -O3 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE
LIB          := -lm

export

CONFIG       := $(DAEMON).conf
SERVICE      := $(DAEMON).service
INSTALL_DIR  := /usr/local/bin
CONFIG_DIR   := /etc
SERVICE_DIR  := /etc/systemd/system

.PHONY: $(COMMON) clean install dirs

all: $(DAEMON) $(CONTROLLER)

$(DAEMON): $(COMMON)
	@$(MAKE) -sC $(SRC_DIR)/$(DAEMON)

$(CONTROLLER): $(COMMON)
	@$(MAKE) -sC $(SRC_DIR)/$(CONTROLLER)

$(COMMON): | dirs
	@$(MAKE) -sC $(SRC_DIR)/$(COMMON)

clean:
	@$(MAKE) clean -sC $(SRC_DIR)/$(DAEMON)
	@$(MAKE) clean -sC $(SRC_DIR)/$(CONTROLLER)
	@$(MAKE) clean -sC $(SRC_DIR)/$(COMMON)
	@rm -rf $(BUILD_DIR)

install:
ifneq (,$(wildcard $(DAEMON)))
	$(info Installing $(DAEMON))
	@install $(DAEMON) $(INSTALL_DIR)/$(DAEMON)
	$(info Copying $(CONFIG))
	@install -m644 $(CONFIG) $(CONFIG_DIR)/$(CONFIG)
	$(info Adding Systemd service $(SERVICE))
	@install -m644 $(SERVICE) $(SERVICE_DIR)/$(SERVIC)
endif
ifneq (,$(wildcard $(CONTROLLER)))
	$(info Installing $(CONTROLLER))
	@install $(CONTROLLER) $(INSTALL_DIR)/$(CONTROLLER)
endif

dirs:
	@mkdir -p $(BUILD_DIR)
