$(info Reading contents.mk $(COMPONENT_DIR))


OFluxSMR.o OFluxSMR.pic.o OFluxThreadNumber.o OFluxThreadNumber.pic.o : CXXFLAGS += $(GCC_G_DEBUG_OPT_TLSUSER)

$(OFLUX_LF_SRC:%.cpp=%.pic.o) $(OFLUX_LF_SRC:%.cpp=%.o) : OPTIMIZATION_FLAGS := -O0
