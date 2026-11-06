COLOR_RESET   := \033[0m
COLOR_RED     := \033[1;31m
COLOR_GREEN   := \033[1;32m
COLOR_YELLOW  := \033[1;33m
COLOR_BLUE    := \033[1;34m
COLOR_MAGENTA := \033[1;35m

ROOT_DIR := $(CURDIR)
SRC_DIR  := $(ROOT_DIR)/src
TEST_DIR := $(ROOT_DIR)/test

PROJECT := trident
ifeq ($(origin CC),default)
CC := clang
endif
MAKEFLAGS += --no-print-directory

DEBUG   := -fsanitize=address,undefined -fno-omit-frame-pointer -g -O0
RELEASE := -O3 -DNDEBUG
CFLAGS  := -Wall -Wextra -Werror
LDFLAGS :=

MODE ?= debug
ifeq ($(MODE),release)
CFLAGS += $(RELEASE)
else ifeq ($(MODE),debug)
CFLAGS  += $(DEBUG)
LDFLAGS += -fsanitize=address,undefined
else
$(error MODE must be debug or release, got '$(MODE)')
endif

BUILD := $(ROOT_DIR)/build/$(MODE)

GIT_TAG       := $(shell git describe --tags --abbrev=0 2>/dev/null || echo 0.0.1)
GIT_HASH      := $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
VERSION       := $(GIT_TAG) $(MODE)-$(shell date +%d%m%Y)-$(GIT_HASH)
TIME_INFO     := $(shell date "+%Y/%m/%d %H:%M:%S:%p")
COMPILER_INFO := $(shell $(CC) --version | head -n 1 | cut -d' ' -f1-3)

VERSION_FLAGS := -DVERSION_INFO='"$(VERSION)"' \
                 -DTIME_INFO='"$(TIME_INFO)"' \
                 -DCC_INFO='"$(COMPILER_INFO)"'

export COLOR_RESET COLOR_RED COLOR_GREEN COLOR_YELLOW COLOR_BLUE COLOR_MAGENTA
export ROOT_DIR SRC_DIR TEST_DIR PROJECT CC CFLAGS LDFLAGS MODE BUILD VERSION_FLAGS

all:
	@if $(MAKE) -q -C src $(ROOT_DIR)/$(PROJECT) >/dev/null 2>&1; then \
		printf '$(COLOR_GREEN)[=] %s is up to date ($(MODE))$(COLOR_RESET)\n' $(PROJECT); \
	else \
		$(MAKE) -C src; \
	fi

check: all golden

golden: all
	@printf '$(COLOR_BLUE)[Runing golden checks]$(COLOR_RESET)\n'
	@ROOT=$(ROOT_DIR) BIN=$(ROOT_DIR)/$(PROJECT) DIR=test/golden python3 $(TEST_DIR)/golden.py

golden-update: all
	@UPDATE=1 ROOT=$(ROOT_DIR) BIN=$(ROOT_DIR)/$(PROJECT) DIR=$(TEST_DIR)/golden python3 $(TEST_DIR)/golden.py

format:
	@$(MAKE) -C src format
	@$(MAKE) -C test format

ifneq ($(findstring clang,$(CC)),clang)
compdb:
	@printf '$(COLOR_RED)[=] compdb requires clang.$(COLOR_RESET)\n'
	@printf '$(COLOR_BLUE)[+] run:$(COLOR_RESET) make compdb CC=clang\n'
else
compdb:
	@$(MAKE) -C src compdb
	@$(MAKE) -C test compdb
	@{ echo '['; cat $(BUILD)/compdb/src.part; echo ','; \
	   cat $(BUILD)/compdb/test.part; echo; echo ']'; } > $(ROOT_DIR)/compile_commands.json
	@printf '$(COLOR_MAGENTA)[=] compile_commands.json$(COLOR_RESET)\n'
endif


clean:
	@printf '$(COLOR_BLUE)[-] Cleaning build artifacts...$(COLOR_RESET)\n'
	@rm -rf $(ROOT_DIR)/build $(ROOT_DIR)/$(PROJECT)

inclusion:
	@python3 inclusion.py

.PHONY: all check golden golden-update unit format compdb clean
