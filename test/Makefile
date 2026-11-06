TBUILD   := $(BUILD)/test

MODE       := debug
SAN        := -fsanitize=address,undefined -fno-omit-frame-pointer
	TFLAGS     := $(CFLAGS) -std=c99 -g -O1 $(SAN) -I$(TEST_DIR) -I$(SRC_DIR) -MMD -MP
TDB_CFLAGS := $(CFLAGS) -std=c99 -I$(TEST_DIR) -I$(SRC_DIR)

TEST_SRCS := $(sort $(shell find $(TEST_DIR) -name 'test_*.c'))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(TBUILD)/obj/%.o,$(TEST_SRCS))
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(TBUILD)/bin/%,$(TEST_SRCS))

LIB_SRCS := $(sort $(shell find $(SRC_DIR) -name '*.c' ! -name main.c))
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(TBUILD)/lib/%.o,$(LIB_SRCS))

DEPS := $(TEST_OBJS:.o=.d) $(LIB_OBJS:.o=.d)
JSON := $(TEST_OBJS:.o=.json) $(LIB_OBJS:.o=.json)

all: check

check: $(TEST_BINS)
	@fail=0; for t in $(TEST_BINS); do \
		printf '$(COLOR_BLUE)[RUN] %s$(COLOR_RESET)\n' $${t#$(TBUILD)/bin/}; \
		$$t || fail=1; \
	done; exit $$fail

$(TBUILD)/bin/%: $(TBUILD)/obj/%.o $(LIB_OBJS)
	@mkdir -p $(dir $@)
	@printf '$(COLOR_GREEN)[+] Linking %s$(COLOR_RESET)\n' $(notdir $@)
	@$(CC) $(TFLAGS) -o $@ $^ $(LDFLAGS)

$(TBUILD)/obj/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@printf '$(COLOR_YELLOW)[CC] %s$(COLOR_RESET)\n' $(patsubst $(TEST_DIR)/%,%,$<)
	@$(CC) $(TFLAGS) -MJ $(@:.o=.json) -c $< -o $@

$(TBUILD)/lib/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@printf '$(COLOR_YELLOW)[CC] %s$(COLOR_RESET)\n' $(patsubst $(SRC_DIR)/%,%,$<)
	@$(CC) $(TFLAGS) -MJ $(@:.o=.json) -c $< -o $@

compdb:
	@mkdir -p $(BUILD)/compdb
	@n=0; for f in $(TEST_SRCS); do \
		if [ $$n -gt 0 ]; then echo ','; fi; n=1; \
		printf '{"directory":"%s","command":"%s","file":"%s"}' \
			"$(ROOT_DIR)" "clang $(TDB_CFLAGS) -c $$f" "$$f"; \
	done > $(BUILD)/compdb/test.part

format:
	@printf '$(COLOR_BLUE)[clang-format] Formatting test files...$(COLOR_RESET)\n'
	@clang-format -i $(TEST_SRCS)

clean-test:
	@rm -rf $(TBUILD)

-include $(DEPS)

.SECONDARY:
.PHONY: all check compdb format clean-test
