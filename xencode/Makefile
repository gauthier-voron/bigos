MAIN   := main.tex
TARGET := xencode.pdf
LATEX  := pdflatex -halt-on-error -interaction nonstopmode
VIEW   := evince

V ?= 1


ifeq ($(V),1)
  define print
    @echo '$1'
  endef
endif

ifneq ($(V),2)
  Q := @
endif

ifneq ($(V),2)
  LATEX  := ! $(LATEX)
  LTXOUT := | grep -E -A 20 '^!'
endif


default: view


all: $(TARGET)
view: $(TARGET)
	$(call print,  VIEW    $<)
	$(Q)$(VIEW) $<


$(TARGET): $(MAIN:.tex=.pdf)
	$(call print,  CP      $@)
	$(Q)cp $(MAIN:.tex=.pdf) $(TARGET)

$(MAIN:.tex=.pdf): $(shell find . -name "*.tex")
	$(call print,  LATEX   $@)
	$(Q)$(LATEX) $(MAIN) $(LTXOUT)
	$(Q)$(LATEX) $(MAIN) $(LTXOUT)


clean:
	$(call print,  CLEAN)
	$(Q)find . -name "*~" -exec rm {} \;
	$(Q)find . -name ".#*" -exec rm {} \;
	$(Q)-rm *.aux *.pdf *.log *.out *.toc 2>/dev/null || true
