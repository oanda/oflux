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
        OFluxLibrary.o

 
liboflux.a: $(OBJS) 

libofshim.so: $(COMMONOBJS)

#dependencies
-include $(OBJS:.o=.d)
