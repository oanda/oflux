##
##
##  OFlux Run time (C++) Makefile
##
##
##

BINDIR= bin

OFILES= \
	$(BINDIR)/OFlux.o \
	$(BINDIR)/OFluxLogging.o \
	$(BINDIR)/OFluxProfiling.o \
	$(BINDIR)/OFluxQueue.o \
	$(BINDIR)/OFluxFlow.o \
	$(BINDIR)/OFluxAtomic.o \
	$(BINDIR)/OFluxAtomicInit.o \
	$(BINDIR)/OFluxAtomicHolder.o \
	$(BINDIR)/OFluxEvent.o \
	$(BINDIR)/OFluxRunTimeAbstract.o \
	$(BINDIR)/OFluxRunTime.o \
	$(BINDIR)/OFluxXML.o

SHIMOFILES= $(BINDIR)/OFluxRunTimeAbstract.pic.o $(BINDIR)/OFluxIOShim.pic.o

OFLUXLIB= $(BINDIR)/liboflux.a
SHIMSO= $(BINDIR)/libofshim.so
ARCH=$(shell uname)
DTRACE=$(shell which dtrace)
ifeq ($(ARCH),Linux)
INCS=
else
INCS= \
	-I/oanda/system/pkg/boost/include/boost-1_34 \
	-I/oanda/system/pkg/expat-1.95.8/include \
	-I/oanda/system/include/boost-1_34_1
endif
LIBDIRS= \
	-L/usr/lib \
	-L/oanda/system/lib \
	-L/oanda/system/pkg/expat-1.95.8/lib
DOXYGEN=$(shell which doxygen | grep -v "^no")
ifeq ($(ARCH),Linux)
LIBS=
else
LIBS= -lposix4 -lexpat -lm -lc -lpthread
endif
CPPOPTS= -g -MMD -MF "$(BINDIR)/.$(notdir $*).d" #-DOFLUX_RT_DEBUG
ifeq ($(ARCH),Linux)
CPPOPTS += -DLINUX
endif
DTRACE=  #temporarily take it out
ifneq ($(DTRACE),)
CPPOPTS += -DHAS_DTRACE
AUTOS= ofluxprobe.h
endif

CXX= g++
CPPOPTS += -DPROFILING 
CPPOPTS += -DTHREAD_COLLECTION #-DSOLARIS_NATIVE_THREADS

all: $(OFLUXLIB) $(SHIMSO)

docs:
	$(if $(DOXYGEN),$(DOXYGEN) oflux.dox, @echo "install doxygen!!")

$(OFLUXLIB): $(OFILES)
	$[ -d $(dir $@) ] || mkdir -p $(dir $@)
ifeq ($(ARCH),SunOS)
	/usr/ccs/bin/ld -r -o bin/glommedobj.o $(OFILES) $(if $(DTRACE),$(BINDIR)/ofluxprobe.o,)
	ar vrcu $(OFLUXLIB) bin/glommedobj.o
else
	ar vrcu $@ $^
endif

$(BINDIR)/%.o : %.cpp 
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(CXX) -c -ftemplate-depth-20 -Wall -D_REENTRANT $(CPPOPTS) $(INCS) $< -o $@
	$(if $(DTRACE),cd ./$(BINDIR); $(DTRACE) -G -s ../ofluxprobe.d $(notdir $@);)

$(BINDIR)/%.pic.o : %.cpp
	@[ -d $(dir $@ ) ] || mkdir -p $(dir $@)
	$(CXX) -c -ftemplate-depth-20 -Wall -D_REENTRANT -fPIC $(CPPOPTS) $(INCS) $< -o $@
	$(if $(DTRACE),cd ./$(BINDIR); $(DTRACE) -G -s ../ofluxprobe.d $(notdir $@);)

$(OFILES): $(AUTOS)

$(SHIMSO): $(SHIMOFILES)
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(CXX) $(LIBDIRS) -shared $(SHIMOFILES) $(LIBS) -o $(SHIMSO) -ldl

ofluxprobe.h: ofluxprobe.d
	$(if $(DTRACE), $(DTRACE) -h -s ofluxprobe.d)

ifneq ($(MAKECMDGOALS),clean)
 -include $(BINDIR)/.*.d
endif

clean:
	rm -f $(BINDIR)/* $(BINDIR)/.*.d ofluxprobe.h
	rm -rf ../../doc/runtime/* 

.PHONY : clean 
