SRC := src/
XEN := xen-4.4/
INC := include/
OBJ := obj/
BIN := bin/

XEN-DIR := ~/xen-4.4/

VENDOR  ?= $(firstword $(shell sed -nr 's/vendor_id[[:space:]]*: (.*)/\1/p' \
                               /proc/cpuinfo))
FAMILY  ?= $(firstword $(shell sed -nr 's/cpu family[[:space:]]*: (.*)/\1/p' \
                               /proc/cpuinfo))
MODEL   ?= $(firstword $(shell sed -nr 's/model[[:space:]]*: (.*)/\1/p' \
                               /proc/cpuinfo))
ARCH    := $(VENDOR)_$(FAMILY)_$(MODEL)

MAJOR_VERSION := 1
MINOR_VERSION := 0
HTFIX_VERSION := 2
VERSION := $(MAJOR_VERSION).$(MINOR_VERSION).$(HTFIX_VERSION)

CC      := gcc
CCFLAGS := -Wall -Wextra
LDFLAGS := -lxenctrl
INCLUDE := -I$(INC)


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


$(BIN)xenperf: $(patsubst $(SRC)%.c, $(OBJ)%.o, $(foreach subdir, $(SRC) \
  $(SRC)$(VENDOR)/ $(SRC)$(VENDOR)/$(FAMILY)/ \
  $(SRC)$(VENDOR)/$(FAMILY)/$(MODEL)/, $(wildcard $(subdir)*.c))) | $(BIN)
	$(call cmd-print,  LD      $@)
	$(Q)$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ)%.o:
	$(call cmd-print,  CC      $@) 
	$(Q)$(CC) $(CCFLAGS) -c $< -o $@ $(INCLUDE) -DVENDOR=$(VENDOR) \
          -DFAMILY=$(FAMILY) -DMODEL=$(MODEL) -DVERSION=$(VERSION)


ifeq ($(mode),build)

-include .depends

endif

.depends: $(foreach subdir, $(SRC) $(SRC)$(VENDOR)/ \
  $(SRC)$(VENDOR)/$(FAMILY)/ $(SRC)$(VENDOR)/$(FAMILY)/$(MODEL)/, \
  $(wildcard $(subdir)*.c)) $(wildcard $(INC)*.h)
	$(call cmd-print,  MKDEP   $@)
	$(Q)$(CC) -MM $(filter %.c, $^) $(INCLUDE) \
        | sed ':x; /\\$$/ { N; s/\\\n[[:space:]]*//; tx }' \
        | sed -r 's%^.*: $(SRC)(.*).c %$$(OBJ)\1.o: $$(SRC)\1.c %' \
        | sed -r 's%^\$$\(OBJ\)(.*/)?(.*):(.*)%$$(OBJ)\1\2:\3 | $$(OBJ)\1%' \
        | sed -r 's/(.*) \| (.*)/\1 | \2\nDIR += \2/' \
	> $@

$(sort $(DIR) $(OBJ) $(BIN)):
	$(call cmd-print,  MKDIR   $@)
	$(Q)mkdir -p $@


clean:
	$(call cmd-print,  CLEAN)
	$(Q)-rm -rf $(OBJ) $(BIN) .depends
	$(Q)find . -name "*~" -exec rm {} \;

endif
