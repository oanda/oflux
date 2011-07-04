$(info Reading contents.mk $(COMPONENT_DIR))

OFLUX_LIB_COMPONENT_DIR:=$(COMPONENT_DIR)

LIBRARIES += liboflux.so libofshim.so
APPS += exercise

OFLUX_SHIMOBJS := OFluxRunTimeAbstractForShim.pic.o OFluxIOShim.pic.o

DTRACECALLINGCPPS := $(if $(DTRACE),$(foreach f,$(shell grep -l "ofluxshimprobe.h\|ofluxprobe.h" `find $(OFLUX_LIB_COMPONENT_DIR) -name '*.cpp'`),$(notdir $(f))),)
DTRACE_FLAGS := -G

OFLUX_LF_SRC := \
  OFluxLockfreeRunTime.cpp \
  OFluxThreadNumber.cpp \
  OFluxLockfreeRunTimeThread.cpp \
  OFluxLFAtomic.cpp \
  OFluxLFAtomicReadWrite.cpp \
  OFluxLFAtomicPooled.cpp \
  OFluxSMR.cpp

OFLUX_LF_OBJS = $(OFLUX_LF_SRC:.cpp=.o)

OFLUX_OBJS = \
        OFlux.o \
        OFluxExceptionsDTrace.o \
        OFluxLogging.o \
        OFluxProfiling.o \
        OFluxQueue.o \
        OFluxOrderable.o \
        OFluxFlow.o \
        OFluxFlowNode.o \
        OFluxFlowNodeIncr.o \
        OFluxFlowCase.o \
        OFluxFlowGuard.o \
        OFluxFlowFunctions.o \
        OFluxFlowExerciseFunctions.o \
        OFluxFlowLibrary.o \
        OFluxAtomic.o \
        OFluxAtomicInit.o \
        OFluxAtomicHolder.o \
        OFluxEarlyRelease.o \
        OFluxEventBase.o \
        OFluxLibDTrace.o \
        OFluxEventOperations.o \
        OFluxRunTimeAbstractForShim.o \
        OFluxRunTimeBase.o \
        OFluxRunTime.o \
        OFluxDoor.o \
        OFluxMeldingRunTime.o \
	$(OFLUX_LF_OBJS) \
        OFluxXML.o \
	oflux_vers.o

# disable optimization for dtrace USDT code 
$(DTRACECALLINGCPPS:%.cpp=%.o) $(DTRACECALLINGCPPS:%.cpp=%.pic.o) : _OPTIMIZATION_FLAGS := $(DTRACE_GCC_OPTIMIZATIONS)
$(DTRACECALLINGCPPS:%.cpp=%.o) $(DTRACECALLINGCPPS:%.cpp=%.pic.o) : OPTIMIZATION_FLAGS := $(DTRACE_GCC_OPTIMIZATIONS)

$(OFLUX_OBJS) $(OFLUX_OBJS:.o=.pic.o) $(OFLUX_SHIMOBJS) : $(DTRACE_LIB_PROBE_HEADER) $(DTRACE_SHIM_PROBE_HEADER)

liboflux.a: $(OFLUX_OBJS)
ifneq ($(DTRACE),)
	$(LD) -r -o glommedobj.o $^
	$(DTRACE) $(DTRACE_FLAGS) -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxprobe.d glommedobj.o -o ofluxprobe_glommed.o
	$(AR) $(ARFLAGS) $@ glommedobj.o ofluxprobe_glommed.o
else
	$(AR) $(ARFLAGS) $@ $^
endif

liboflux.so: $(OFLUX_OBJS:%.o=%.pic.o)
ifneq ($(DTRACE),)
	$(foreach f,$(shell grep -l "include.*probe.h" `find $(OFLUX_LIB_COMPONENT_DIR) -name '*.h' | grep -v OFluxLibDTrace.h`), $(error must not expose OFluxLibDTrace.h or *probe.h in headers : $(f))) \
	$(LD) -r -o glommedobj_so.o $^
	$(DTRACE) $(DTRACE_FLAGS) -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxprobe.d glommedobj_so.o -o ofluxprobe_glommed_so.o
	$(CXX) -shared glommedobj_so.o ofluxprobe_glommed_so.o $(OFLUXRTLIBS) -o $@
else
	$(CXX) -shared $^ $(OFLUXRTLIBS) -o $@
endif

$(DTRACE_LIB_PROBE_HEADER): ofluxprobe.d
	$(if $(DTRACE), $(DTRACE) -h -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxprobe.d)

$(DTRACE_SHIM_PROBE_HEADER): ofluxshimprobe.d
	$(if $(DTRACE), $(DTRACE) -h -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxshimprobe.d)


.SECONDARY: $(OFLUX_OBJS) $(OFLUX_OBJS:%.o=%.pic.o)

ifeq ($(_ARCH),SunOS)
OFLUXRTLIBS= -lposix4 -lexpat -lm -lc -lpthread
else ifeq ($(_ARCH),Linux)
OFLUXRTLIBS=
else ifeq ($(_ARCH),Darwin)
OFLUXRTLIBS= -lexpat -lm -lc -lpthread
endif

libofshim.so: $(OFLUX_SHIMOBJS)
	$(if $(DTRACE),$(DTRACE) $(DTRACE_FLAGS) -s $(OFLUX_LIB_COMPONENT_DIR)/ofluxshimprobe.d OFluxIOShim.pic.o -o ofluxshimprobe_so.o)
ifeq  ($(_ARCH),Darwin)
	$(CXX) -flat_namespace -dynamiclib -shared -Wl $^ -o $@
else
	$(CXX) -shared -Wl,-z,interpose $^ $(if $(DTRACE),ofluxshimprobe_so.o,) $(OFLUXRTLIBS) -o $@
endif
OFLUX_LIB_VERS_READ:=$(shell test -r $(CURDIR)/oflux_vers.cpp && grep "^\"v" $(CURDIR)/oflux_vers.cpp | sed s/\"//g)
OFLUX_LIB_VERS_EXISTING:=$(shell cd $(OFLUX_LIB_COMPONENT_DIR); git describe --tags --match 'v*'; cd $(CURDIR))

ifeq ($(OFLUX_LIB_VERS_READ), $(OFLUX_LIB_VERS_EXISTING))
  VERSDEPEND=
else
  VERSDEPEND=FORCE
endif

FORCE:

oflux_vers.cpp: $(VERSDEPEND)
	(echo "/* auto-generated - do not modify */" \
	; echo "" \
	; echo " namespace oflux { const char * runtime_version = " \
	; cd $(OFLUX_LIB_COMPONENT_DIR) \
	; echo "\""`git describe --tags --match 'v*'`"\"" \
	;cd $(CURDIR) \
	; echo "; }" \
	; echo "") > oflux_vers.cpp

exercise : oflux_exercise.o liboflux.so libofshim.so
	$(CXX) $(CXXOPTS) $(CXXFLAGS) $(INCS) $(LIBDIRS) $^ $(LIBS) -o $@

exercise : LIBS += $(UMEMLIB)

oflux_exercise.o: $(OFLUXPROBEHEADER)

OFLUX_DOCUMENTATION += doc/runtime

doc/runtime: oflux.dox $(OFLUX_OBJS) oflux_vers.cpp
	mkdir -p $@; \
	$(DOXYGEN) $<

#dependencies
-include $(OFLUX_OBJS:.o=.depend)
