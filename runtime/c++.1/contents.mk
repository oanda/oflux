$(warning Reading contents.mk)

OFLUX_LIB_COMPONENT_DIR:=$(COMPONENT_DIR)

LIBRARIES += liboflux.a libofshim.so 

COMMONOBJS := OFluxRunTimeAbstract.pic.o OFluxIOShim.pic.o

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
        OFluxRunTime.o \
        OFluxMeldingRunTime.o \
        OFluxXML.o \
        OFluxLibrary.o \
	oflux_vers.o

liboflux.a: liboflux.a($(OBJS))

liboflux.so: $(OBJS:%.o=%.pic.o)
	$(CXX) -shared $^ $(OFLUXRTLIBS) -o $@

.SECONDARY: $(OBJS) $(OBJS:%.o=%.pic.o)

ifeq ($(_ARCH),Linux)
OFLUXRTLIBS=
else
OFLUXRTLIBS= -lposix4 -lexpat -lm -lc -lpthread
endif

libofshim.so: $(COMMONOBJS) -ldl
	$(CXX) -shared $^ $(OFLUXRTLIBS) -o $@

OFLUX_LIB_VERS_READ:=$(shell test -r $(CURDIR)/oflux_vers.cpp && grep "^\"v" $(CURDIR)/oflux_vers.cpp | sed s/\"//g)
OFLUX_LIB_VERS_EXISTING:=$(shell cd $(OFLUX_LIB_COMPONENT_DIR); git describe --tags; cd $(CURDIR))

ifeq ($(OFLUX_LIB_VERS_READ), $(OFLUX_LIB_VERS_EXISTING))
  VERSDEPEND=
else
  VERSDEPEND=FORCE
endif

FORCE:

oflux_vers.cpp: $(VERSDEPEND)
	(echo "/* auto-generated - do not modify */"; echo ""; echo " namespace oflux { const char * runtime_version = "; cd $(OFLUX_LIB_COMPONENT_DIR); echo "\""`git describe --tags`"\"";cd $(CURDIR); echo "; }"; echo "") > oflux_vers.cpp


#dependencies
-include $(OBJS:.o=.d)
