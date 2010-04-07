$(info Reading ex-contents.mk $(COMPONENT_DIR))


OFLUX_PROJECT_NAME:=basickernel

$(OFLUX_PROJECT_NAME)_OFLUX_KERNEL:=$(OFLUX_PROJECT_NAME).cpp

OFLUX_PROJECTS+= $(OFLUX_PROJECT_NAME)
OFLUX_TESTS+= run-$(OFLUX_PROJECT_NAME).sh

$(OFLUX_PROJECT_NAME)_OFLUX_MODULES:=
$(OFLUX_PROJECT_NAME)_OFLUX_OPTS+= -x

$(OFLUX_PROJECT_NAME)_OFLUX_MAIN:=$(OFLUX_PROJECT_NAME).flux

$(OFLUX_PROJECT_NAME)_OFLUX_SRC:=mImpl_$(OFLUX_PROJECT_NAME).cpp
$(OFLUX_PROJECT_NAME)_OFLUX_CXXFLAGS:= 

############################################################################
## General stuff for build tests


$(OFLUX_PROJECT_NAME)_OFLUX_PATH:=$(COMPONENT_DIR)
$(OFLUX_PROJECT_NAME)_OFLUX_INCS:=-I $($(OFLUX_PROJECT_NAME)_OFLUX_PATH)

$(OFLUX_PROJECT_NAME)_OFLUX_INCLUDES:=

kernalex-all: run-basickernel.sh basicplug1_done basicplug2_done basicplug3_done

