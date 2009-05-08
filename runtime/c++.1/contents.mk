$(warning Reading contents.mk)

OFLUX_LIB_COMPONENT_DIR:=$(COMPONENT_DIR)

LIBRARIES += liboflux.a libofshim.so 

COMMONOBJS := OFluxRunTimeAbstract.pic.dt.o OFluxIOShim.pic.dt.o

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


$(OBJS) $(OBJS:.o=.pic.o) $(COMMONOBJS) : $(DTRACE_PROBE_HEADER)

liboflux.a: $(OBJS:.o=.dt.o) $(DTRACE_PROBE_OBJ)
ifeq ($(_ARCH),SunOS)
	$(LD) $(LDFLAGS) -r -o glommedobj.o $^ 
	$(AR) r $@ glommedobj.o
else
	$(AR) r $@ $^ $(DTRACE_PROBE_OBJ)
endif

liboflux.so: $(OBJS:%.o=%.pic.dt.o) $(DTRACE_PROBE_OBJ)
ifeq ($(_ARCH),SunOS)
	$(LD) $(LDFLAGS) -r -o glommedobj.pic.o $^ 
	$(LD) $(LDFLAGS) -G -o $@ glommedobj.pic.o \
		$(OFLUXRTLIBS)
else
	$(CXX) -shared $^ $(OFLUXRTLIBS) -o $@
endif

$(DTRACE_PROBE_HEADER): ofluxprobe.d
	$(if $(DTRACE), $(DTRACE) -h -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxprobe.d)

%.dt.o: %.o ofluxprobe.d
	cp $< $@ 
	$(if $(DTRACE),$(DTRACE) -G -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxprobe.d \
		$@,)
		#$(if $(filter $<,$(OBJS)), -o ofluxprobe.o,) $@,)

.SECONDARY: $(OBJS) $(OBJS:%.o=%.pic.o)

ifeq ($(_ARCH),Linux)
OFLUXRTLIBS=
else
OFLUXRTLIBS= -lposix4 -lexpat -lm -lc -lpthread
endif

libofshim.so: $(COMMONOBJS) -ldl
	$(CXX) -shared -Wl,-z,interpose $^ $(OFLUXRTLIBS) -o $@

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
-include $(OBJS:.o=.d)
