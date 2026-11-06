TARGET := $(ROOT_DIR)/$(PROJECT)

SRCS := $(sort $(shell find $(SRC_DIR) -name '*.c'))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

CPPFLAGS_ALL := -I$(SRC_DIR) -MMD -MP
ALL_CFLAGS   := $(CFLAGS) -std=c99 $(CPPFLAGS_ALL)
DB_CFLAGS    := $(CFLAGS) -std=c99 -I$(SRC_DIR)

LEXER_DIR  := $(SRC_DIR)/syntax/lexer
KEYWORDS   := $(LEXER_DIR)/keywords
INTRINSICS := $(LEXER_DIR)/intrinsics
GENERATED  := $(KEYWORDS).h $(INTRINSICS).h

all: $(TARGET)

$(OBJS): | $(GENERATED)

$(KEYWORDS).h: $(KEYWORDS).def
	@printf '$(COLOR_MAGENTA)[gperf] %s$(COLOR_RESET)\n' $(notdir $@)
	@gperf -N get_keyword_kind -t $< > $@.tmp && mv $@.tmp $@ || { rm -f $@.tmp; exit 1; }

$(INTRINSICS).h: $(INTRINSICS).def
	@printf '$(COLOR_MAGENTA)[gperf] %s$(COLOR_RESET)\n' $(notdir $@)
	@gperf -N get_intrinsic_kind -t $< > $@.tmp && mv $@.tmp $@ || { rm -f $@.tmp; exit 1; }

$(TARGET): $(OBJS)
	@printf '$(COLOR_GREEN)[+] Linking %s ($(MODE))$(COLOR_RESET)\n' $(notdir $@)
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@printf '$(COLOR_YELLOW)[CC] %s$(COLOR_RESET)\n' $(patsubst $(SRC_DIR)/%,%,$<)
	@$(CC) $(ALL_CFLAGS) -c $< -o $@

compdb:
	@mkdir -p $(BUILD)/compdb
	@n=0; for f in $(SRCS); do \
		if [ $$n -gt 0 ]; then echo ','; fi; n=1; \
		printf '{"directory":"%s","command":"%s","file":"%s"}' \
			"$(ROOT_DIR)" "clang $(DB_CFLAGS) -c $$f" "$$f"; \
	done > $(BUILD)/compdb/src.part

format:
	@printf '$(COLOR_BLUE)[clang-format] Formatting source files...$(COLOR_RESET)\n'
	@clang-format -i $(SRCS)

clean-gen:
	@rm -f $(GENERATED)

-include $(DEPS)

.PHONY: all compdb format clean-gen
