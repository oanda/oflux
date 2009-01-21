$(warning Make v:$(MAKE_VERSION) Starting oflux main makefile, goal:$(MAKECMDGOALS))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
include Mk/target.mk
else
#----- Begin Boilerplate

RELEASE := production
VPATH := $(SRCDIR)
 
include $(SRCDIR)/Mk/rules.mk

all: build

#begin debug helpers

print-%: ; @echo $* is $($*)

OLD_SHELL := $(SHELL)
	SHELL = $(warning [$@ ($^)($?)])$(OLD_SHELL)

#end debug helpers

DIR_SPECIFIC_MAKEFILE_NAME := contents.mk

buildable_dirs = \
	$(shell find $(SRCDIR) -name $(DIR_SPECIFIC_MAKEFILE_NAME) | \
	sed 's/$(DIR_SPECIFIC_MAKEFILE_NAME)//')


define process_dir
  LIBRARIES :=

  # passed to the contents.mk module files
  COMPONENT_DIR = $1
  include $1/contents.mk
  ALL_TOOLS += $$(TOOLS)
  ALL_LIBRARIES += $$(LIBRARIES)
  ALL_APPS += $$(APPS)
  VPATH += $1
  #
  # The last line of the function must be left blank
  # in order to avoid some quirky, broken gmake
  # behavior when expanding macros within foreach
  # loops.
  #

endef


define process_dirs
  $(foreach DIR, $(buildable_dirs),\
	$(call process_dir,$(DIR)))
endef


define process_oflux_project
#VPATH:=.:$(2)
$(1)_OFLUX_MODULE_OBJS=$$($(1)_OFLUX_MODULES:%.flux=OFluxGenerate_$(1)_%.o)
$(1)_OFLUX_MODULE_CPPS=$$($(1)_OFLUX_MODULES:%.flux=OFluxGenerate_$(1)_%.cpp)
OFluxGenerate_$(1)_%.h OFluxGenerate_$(1)_%.cpp : %.flux mImpl_%.h oflux
	$(OFLUXCOMPILER) -oprefix OFluxGenerate_$(1) -a $$* $$($(1)_OFLUX_INCS) $$<
OFluxGenerate_$(1).h OFluxGenerate_$(1).cpp : $($(1)_OFLUX_MAIN) mImpl.h $$($(1)_OFLUX_MODULE_CPPS) oflux
	$(OFLUXCOMPILER) -oprefix OFluxGenerate_$(1) $$($(1)_OFLUX_INCS) $$($(1)_OFLUX_MAIN)
$$($(1)_OFLUX_MAIN:%.flux=%) : $$($(1)_OFLUX_MODULE_OBJS)
$$($(1)_OFLUX_MODULE_OBJS) : INCS = $(INCS) -I$$($(1)_OFLUX_PATH)
OFluxGenerate_$(1).o : INCS = $(INCS) -I$$($(1)_OFLUX_PATH)
#
# no link rule done

endef

define process_oflux_projects
  $(foreach PROJ,$(OFLUX_PROJECTS),\
	$(call process_oflux_project,$(notdir $(PROJ)),$(dir $(PROJ))))
endef

ALL_LIBRARIES :=

$(eval $(process_dirs))
$(eval $(process_oflux_projects))

ALL_TESTS := $(OFLUX_TESTS)

.PHONY:
	all \
	clean

build: $(ALL_TOOLS) $(ALL_LIBRARIES) $(ALL_APPS) $(ALL_TESTS)

clean::
	$(RM) $(ALL_LIBRARIES)

#----- END Boilerplate
endif

