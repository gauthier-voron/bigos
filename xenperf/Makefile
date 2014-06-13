SRC := src/
XEN := xen-4.4/
INC := include/
OBJ := obj/
BIN := bin/

ARCH    := intel/
XEN-DIR := ~/xen-4.4/

CC      := gcc
CCFLAGS := -Wall -Wextra
LDFLAGS := -lxenctrl


V ?= 1
ifeq ($(V),1)
  define cmd-print
    @echo "$1"
  endef
endif
ifneq ($(V),2)
  Q := @
endif

MAKEFLAGS += -rR --no-print-directory

config-target := clean .depends
ifeq ($(filter $(config-target), $(MAKECMDGOALS)),)
  mode := build
else
  ifeq ($(filter-out $(config-target), $(MAKECMDGOALS)),)
    mode := config
  else
    mode := mix
  endif
endif


ifeq ($(mode),mix)

%::
	$(Q)+$(MAKE) $@

else

default: check

all: $(BIN)xenperf
check: $(BIN)xenperf
	$(call cmd-print,  CHECK   $<)
	$(Q)sudo ./$<


xen: $(XEN-DIR)dist/install/boot/
$(XEN-DIR)dist/install/boot/: $(shell find $(XEN) -name "*.c")
	rsync -aAHXvz --exclude="*~" $(XEN) $(XEN-DIR)
	make -C $(XEN-DIR) -j 4 --no-print-directory dist-xen
	sudo make -C $(XEN-DIR) -j 4 --no-print-directory install-xen
	sudo reboot


$(BIN)xenperf: $(patsubst $(SRC)%.c, $(OBJ)%.o, $(wildcard $(SRC)*.c)) \
       $(patsubst $(SRC)$(ARCH)%.c, $(OBJ)%.o, $(wildcard $(SRC)$(ARCH)*.c)) \
       | $(BIN)
	$(call cmd-print,  LD      $@)
	$(Q)$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ)%.o:
	$(call cmd-print,  CC      $@) 
	$(Q)$(CC) $(CCFLAGS) -c $< -o $@ -I$(INC)


ifeq ($(mode),build)

-include .depends

.depends: $(wildcard $(SRC)*.c) $(wildcard $(SRC)$(ARCH)*.c) \
          $(wildcard $(INC)*.h)
	$(call cmd-print,  MKDEP   $@)
	$(Q)$(CC) -MM $(filter %.c, $^) -I$(INC) \
        | sed -r 's/(.*):/$$(OBJ)\1:/' \
        | sed -r 's/([^\\])[[:space:]]*$$/\1 | $$(OBJ)/' \
        >$@

endif


$(OBJ) $(BIN):
	$(call cmd-print,  MKDIR   $@)
	$(Q)mkdir $@


clean:
	$(call cmd-print,  CLEAN)
	$(Q)-rm -rf $(OBJ) $(BIN) .depends
	$(Q)find . -name "*~" -exec rm {} \;

endif
