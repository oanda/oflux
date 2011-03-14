$(info Reading contents.mk $(COMPONENT_DIR))

SPECIAL_OBJS_SPARC_LINKING := \
  OFluxSMR \
  OFluxThreadNumber \
  OFluxLockfreeRunTime

$(SPECIAL_OBJS_SPARC_LINKING:%=%.o) $(SPECIAL_OBJS_SPARC_LINKING:%=%.pic.o) : CXXFLAGS += $(GCC_G_DEBUG_OPT_TLSUSER)

$(OFLUX_LF_SRC:%.cpp=%.pic.o) $(OFLUX_LF_SRC:%.cpp=%.o) : OPTIMIZATION_FLAGS := -O0
