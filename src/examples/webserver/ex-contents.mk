$(info Reading ex-contents.mk $(COMPONENT_DIR))


OFLUX_PROJECT_NAME:=webserver
include $(SRCDIR)/Mk/oflux_example.mk

$(OFLUX_PROJECT_NAME)_LOADTEST_ARGS:= 8080 ../../

