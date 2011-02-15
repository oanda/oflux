$(info Reading ex-contents.mk $(COMPONENT_DIR))


OFLUX_PROJECT_NAME:=orderingtest


$(OFLUX_PROJECT_NAME)_TEST_RESULT:= orderingtest_load_test

OFLUX_TESTS+= $($(OFLUX_PROJECT_NAME)_TEST_RESULT)

include $(SRCDIR)/Mk/oflux_example.mk

#$($(OFLUX_PROJECT_NAME)_TEST_RESULT): run-$(OFLUX_PROJECT_NAME).sh
#	./$< > $@


