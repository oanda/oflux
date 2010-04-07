$(info Reading ex-contents.mk $(COMPONENT_DIR))


OFLUX_PROJECT_NAME:=condguardtest2

include $(SRCDIR)/Mk/oflux_example.mk

$(OFLUX_PROJECT_NAME)_OFLUX_CXXFLAGS+= -DHASINIT
