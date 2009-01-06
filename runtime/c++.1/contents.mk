$(warning Reading contents.mk)

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
        OFluxEvent.o \
        OFluxRunTimeAbstract.o \
        OFluxRunTime.o \
        OFluxXML.o \
        OFluxLibrary.o \
	oflux_vers.o

 
liboflux.a: $(OBJS) 

libofshim.so: $(COMMONOBJS)

ifeq ($(shell grep "^\"v" $(CURDIR)/oflux_vers.cpp | sed s/\"//g), $(shell git describe --tags))
  VERSDEPEND=
else
  VERSDEPEND=FORCE
endif

FORCE:

oflux_vers.cpp: $(VERSDEPEND)
	(echo "/* auto-generated - do not modify */"; echo ""; echo " namespace oflux { const char * runtime_version = "; echo "\""`git describe --tags`"\"";echo "; }"; echo "") > oflux_vers.cpp


#dependencies
-include $(OBJS:.o=.d)
