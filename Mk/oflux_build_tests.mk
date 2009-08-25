
$(OFLUX_PROJECT_NAME)_OFLUX_GEN_HEADERS:= \
	OFluxGenerate_$(OFLUX_PROJECT_NAME).h \
	$(foreach m,$($(OFLUX_PROJECT_NAME)_OFLUX_MODULES:%.flux=%),OFluxGenerate_$(OFLUX_PROJECT_NAME)_$(m).h)
$(OFLUX_PROJECT_NAME)_OFLUX_SRC:=mImpl_$(OFLUX_PROJECT_NAME).cpp $($(OFLUX_PROJECT_NAME)_OFLUX_KERNEL)
$(OFLUX_PROJECT_NAME)_OFLUX_CXXFLAGS:= -DTESTING

$(OFLUX_PROJECT_NAME)_OFLUX_PATH:=$(COMPONENT_DIR)
$(OFLUX_PROJECT_NAME)_OFLUX_INCS:=-I $($(OFLUX_PROJECT_NAME)_OFLUX_PATH)

$(OFLUX_PROJECT_NAME)_OFLUX_INCLUDES:=

$($(OFLUX_PROJECT_NAME)_OFLUX_MODULES:%.flux=mImpl_%.h): %.h: $($(OFLUX_PROJECT_NAME)_OFLUX_PATH)/mImpl.h
	ln -sf $< $@

$($(OFLUX_PROJECT_NAME)_OFLUX_KERNEL:%.flux=mImpl_%.h): $($(OFLUX_PROJECT_NAME)_OFLUX_PATH)/mImpl.h
	ln -sf $< $@

mImpl_$(OFLUX_PROJECT_NAME).cpp: $($(OFLUX_PROJECT_NAME)_OFLUX_GEN_HEADERS)
	(cat $? | awk -f $(SRCDIR)/tests/build/create_cpp.awk -v proj=$(subst mImpl_,,$(basename $@))) > $@

