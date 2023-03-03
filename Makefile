MAKEFLAGS += --no-builtin-rules

SPACE:=$(subst ,, )

SRC:=src
OBJ:=obj

CC:=clang
CFLAGS:=-Werror -Wall -Wextra -Wconversion -std=gnu17 -I$(SRC) -fPIC -flto -Ofast $(D)
LDFLAGS:=-flto -fPIE -Ofast
RELEASE_FLAGS:=-g0 -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident
DEBUG_FLAGS:=-fsanitize=address -fsanitize=undefined -g

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

C_FILE:=$(strip $(call rwildcard,$(SRC),*.c))
O_FILE:=$(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(C_FILE))
O_FILE_D:=$(patsubst %.o,%.debug.o,$(O_FILE))

TARGET:=cfg-code

ifeq ($(V),)
cmd=@$(call cmd_$(1)_name,$(2),$(3)); $(call cmd_$(1),$(2),$(3))
else
cmd=$(call cmd_$(1),$(2),$(3))
endif

cmd_rm_name=printf '  %-6s %s\n' "RM" "$(1)"
cmd_rm=rm -rf $(1)

cmd_cc_rl_name=printf '  %-6s %s -> %s\n' "CC" "$(1)" "$(2)"
cmd_cc_rl=mkdir -p $(dir $(2)); $(CC) $(CFLAGS) $(RELEASE_FLAGS) -c $(1) -o $(2)

cmd_ld_rl_name=printf '  %-6s %s -> %s\n' "LD" "$(1)" "$(2)"
cmd_ld_rl=$(CC) $(LDFLAGS) $(RELEASE_FLAGS) $(1) -o $(2)

cmd_cc_db_name=printf '  %-6s %s -> %s\n' "CC" "$(1)" "$(2)"
cmd_cc_db=mkdir -p $(dir $(2)); $(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $(1) -o $(2)

cmd_ld_db_name=printf '  %-6s %s -> %s\n' "LD" "$(1)" "$(2)"
cmd_ld_db=$(CC) $(LDFLAGS) $(DEBUG_FLAGS) $(1) -o $(2)

.SUFFIXES:
.SECONDARY:
.PHONY: all debug clean

all: $(TARGET)

debug: $(TARGET).debug

clean:
	$(call cmd,rm,$(TARGET) $(TARGET).debug $(OBJ))

$(TARGET): $(O_FILE)
	$(call cmd,ld_rl,$(O_FILE),$(TARGET))

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(call cmd,cc_rl,$^,$@)

$(TARGET).debug: $(O_FILE_D)
	$(call cmd,ld_db,$^,$@)

$(OBJ)/%.debug.o: $(SRC)/%.c | $(OBJ)
	$(call cmd,cc_db,$^,$@)

$(OBJ):
	@mkdir -p $@
