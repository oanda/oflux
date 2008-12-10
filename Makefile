$(warning Make v:$(MAKE_VERSION) Starting xs-libraries main makefile, goal:$(MAKECMDGOALS))

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

#OLD_SHELL := $(SHELL)
#	SHELL = $(warning [$@ ($^)($?)])$(OLD_SHELL)

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

ALL_LIBRARIES :=

$(eval $(process_dirs))

.PHONY:
	all \
	clean

build: $(ALL_TOOLS) $(ALL_LIBRARIES) $(ALL_APPS)

clean::
	$(RM) $(ALL_LIBRARIES)

#----- END Boilerplate
endif

