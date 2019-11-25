CC           ?= gcc

BASE_DIR 	 := $(shell pwd)
COMMON_DIR	 := common

DAEMON		 := amdgpu-fand
CONTROLLER	 := amdgpu-fanctl

SRC_DIR		 := src
BUILD_DIR    := $(BASE_DIR)/build
SRC_EXT      := c
OBJ_EXT      := o
CFLAGS       := -std=c11 -Wall -Wextra -pedantic -Wshadow -Wunknown-pragmas -O3 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE
LIB          := -lm

SHARED_VARS  := G_CFLAGS="$(CFLAGS)"       \
				G_LIB="$(LIB)"		   	   \
				BUILD_DIR="$(BUILD_DIR)"   \
				BASE_DIR="$(BASE_DIR)"     \
				SRC_EXT="$(SRC_EXT)"       \
				OBJ_EXT="$(OBJ_EXT)"	   \
				COMMON_DIR="$(COMMON_DIR)"

CONFIG       := $(DAEMON).conf
SERVICE      := $(DAEMON).service
INSTALL_DIR  := /usr/local/bin
CONFIG_DIR   := /etc
SERVICE_DIR  := /etc/systemd/system

.PHONY: $(DAEMON) $(CONTROLLER) clean install

all: $(DAEMON) $(CONTROLLER)

$(DAEMON):
	@$(MAKE) -sC $(SRC_DIR)/$(DAEMON) $(SHARED_VARS)

$(CONTROLLER):
	@$(MAKE) -sC $(SRC_DIR)/$(CONTROLLER) $(SHARED_VARS)

clean:
	@$(MAKE) clean -sC $(SRC_DIR)/$(DAEMON) $(SHARED_VARS)
	@$(MAKE) clean -sC $(SRC_DIR)/$(CONTROLLER) $(SHARED_VARS)

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
