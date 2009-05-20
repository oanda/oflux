$(warning Reading contents.mk)

OFLUX_LIB_COMPONENT_DIR:=$(COMPONENT_DIR)

LIBRARIES += liboflux.a libofshim.so 

SHIMOBJS := OFluxRunTimeAbstract.pic.o OFluxIOShim.pic.o

DTRACECALLINGCPPS := $(foreach f,$(shell grep -l "OFluxLibDTrace.h\|ofluxshimprobe.h" $(OFLUX_LIB_COMPONENT_DIR)/*.cpp),$(notdir $(f)))

OBJS := \
        OFlux.o \
        OFluxLogging.o \
        OFluxProfiling.o \
        OFluxQueue.o \
        OFluxOrderable.o \
        OFluxFlow.o \
        OFluxAtomic.o \
        OFluxAtomicInit.o \
        OFluxAtomicHolder.o \
        OFluxAcquireGuards.o \
        OFluxEvent.o \
        OFluxRunTimeAbstract.o \
        OFluxRunTimeBase.o \
        OFluxRunTime.o \
        OFluxMeldingRunTime.o \
        OFluxXML.o \
        OFluxLibrary.o \
	oflux_vers.o

# disable optimization for dtrace USDT code (PIC code not affected):
$(DTRACECALLINGCPPS:.cpp=.o) : OPTIMIZATION_FLAGS := $(DTRACE_GCC_OPTIMIZATIONS)

$(OBJS) $(OBJS:.o=.pic.o) $(SHIMOBJS) : $(DTRACE_LIB_PROBE_HEADER) $(DTRACE_SHIM_PROBE_HEADER)

liboflux.a: $(OBJS) 
ifneq ($(DTRACE),)
	$(LD) -r -o glommedobj.o $^
	$(DTRACE) -G -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxprobe.d glommedobj.o -o ofluxprobe_glommed.o
	$(AR) r $@ glommedobj.o ofluxprobe_glommed.o
else
	$(AR) r $@ $^ 
endif

liboflux.so: $(OBJS:%.o=%.pic.o)
ifneq ($(DTRACE),)
	$(LD) -r -o glommedobj_so.o $^
	$(DTRACE) -G -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxprobe.d glommedobj_so.o -o ofluxprobe_glommed_so.o
	$(CXX) -shared glommedobj_so.o ofluxprobe_glommed_so.o $(OFLUXRTLIBS) -o $@
else
	$(CXX) -shared $^ $(OFLUXRTLIBS) -o $@
endif

$(DTRACE_LIB_PROBE_HEADER): ofluxprobe.d
	$(if $(DTRACE), $(DTRACE) -h -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxprobe.d)

$(DTRACE_SHIM_PROBE_HEADER): ofluxshimprobe.d
	$(if $(DTRACE), $(DTRACE) -h -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxshimprobe.d)


.SECONDARY: $(OBJS) $(OBJS:%.o=%.pic.o)

ifeq ($(_ARCH),Linux)
OFLUXRTLIBS=
else
OFLUXRTLIBS= -lposix4 -lexpat -lm -lc -lpthread
endif

libofshim.so: $(SHIMOBJS) -ldl
	$(if $(DTRACE),$(DTRACE) -G -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxshimprobe.d OFluxIOShim.pic.o -o ofluxshimprobe_so.o)
	$(CXX) -shared -Wl,-z,interpose $^ $(if $(DTRACE),ofluxshimprobe_so.o,) $(OFLUXRTLIBS) -o $@

OFLUX_LIB_VERS_READ:=$(shell test -r $(CURDIR)/oflux_vers.cpp && grep "^\"v" $(CURDIR)/oflux_vers.cpp | sed s/\"//g)
OFLUX_LIB_VERS_EXISTING:=$(shell cd $(OFLUX_LIB_COMPONENT_DIR); git describe --tags; cd $(CURDIR))

ifeq ($(OFLUX_LIB_VERS_READ), $(OFLUX_LIB_VERS_EXISTING))
  VERSDEPEND=
else
  VERSDEPEND=FORCE
endif

FORCE:

oflux_vers.cpp oflux_vers.h: $(VERSDEPEND)
	(echo "/* auto-generated - do not modify */" \
	; echo "" \
	; echo " namespace oflux { const char * runtime_version = " \
	; cd $(OFLUX_LIB_COMPONENT_DIR) \
	; echo "\""`git describe --tags`"\"" \
	;cd $(CURDIR) \
	; echo "; }" \
	; echo "") > oflux_vers.cpp
	( echo "/**" \
	; echo " * @version "`git describe --tags | sed s/v// | sed s/-.*//g` \
	; echo " */") > oflux_vers.h

OFLUX_DOCUMENTATION += doc/runtime

doc/runtime: oflux.dox $(OBJS) oflux_vers.cpp
	mkdir -p $@; \
	$(DOXYGEN) $<

#dependencies
-include $(OBJS:.o=.depend)
