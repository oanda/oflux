$(info reading ex-contents.mk $(COMPONENT_DIR))

OFLUX_UNIT_TESTS += \
  OFluxOrderable_unittest.cpp \
  OFluxEvent_unittest.cpp \
  OFluxLinkedList_unittest.cpp \
  OFluxAtomic_unittest.cpp

OFluxEvent_unittest OFluxAtomic_unittest : CommonEventunit.o
