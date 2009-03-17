
# Define ARCH_FLAGS for architecture specific compile flags

ARCH_FLAGS += -DLINUX
LIBS += -lrt
DOTCOMMAND:= $(shell which dot | grep -v no)
DOT:=$(if $(DOTCOMMAND),$(DOTCOMMAND) -Tsvg,)
