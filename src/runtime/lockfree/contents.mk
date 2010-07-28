$(info Reading contents.mk $(COMPONENT_DIR))

OFLUX_LF_SRC := \
  OFluxLockfreeRunTime.cpp \
  OFluxThreadNumber.cpp \
  OFluxLockfreeRunTimeThread.cpp \
  OFluxLFAtomic.cpp

OFluxThreadNumber.o OFluxThreadNumber.pic.o : CXXFLAGS += $(GCC_G_DEBUG_OPT_TLSUSER)

$(OFLUX_LF_SRC:%.cpp=%.pic.o) $(OFLUX_LF_SRC:%.cpp=%.o) : OPTIMIZATION_FLAGS := -O0
