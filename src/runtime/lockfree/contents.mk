$(info Reading contents.mk $(COMPONENT_DIR))

OFluxThreadNumber.o OFluxThreadNumber.pic.o : CXXFLAGS += $(GCC_G_DEBUG_OPT_TLSUSER)
