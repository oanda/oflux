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
	$(BINDIR)/OFluxOrderable.o \
	$(BINDIR)/OFluxFlow.o \
	$(BINDIR)/OFluxAtomic.o \
	$(BINDIR)/OFluxAtomicInit.o \
	$(BINDIR)/OFluxAtomicHolder.o \
        $(BINDIR)/OFluxAcquireGuards.o \
	$(BINDIR)/OFluxEvent.o \
	$(BINDIR)/OFluxRunTimeAbstract.o \
	$(BINDIR)/OFluxRunTime.o \
	$(BINDIR)/OFluxMeldingRunTime.o \
	$(BINDIR)/OFluxXML.o \
	$(BINDIR)/OFluxLibrary.o \
	$(BINDIR)/oflux_vers.o

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
CPPOPTS= -g -MMD -MF "$(BINDIR)/.$(@F:.o=.depend)" #-DOFLUX_RT_DEBUG
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

ifeq ($(shell test -e oflux_vers.cpp && grep "^\"v" oflux_vers.cpp | sed s/\"//g), $(shell git describe --tags))
  VERSDEPEND=
else
  VERSDEPEND=FORCE
endif

oflux_vers.cpp: $(VERSDEPEND)
	(echo "/* auto-generated - do not modify */"; echo ""; echo " namespace oflux { const char * runtime_version = "; echo "\""`git describe --tags`"\"";echo "; }"; echo "") > oflux_vers.cpp


$(BINDIR)/%.o : %.cpp 
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(CXX) -c -Wall -D_REENTRANT $(CPPOPTS) $(INCS) $< -o $@
	$(if $(DTRACE),cd ./$(BINDIR); $(DTRACE) -G -s ../ofluxprobe.d $(notdir $@);)

$(BINDIR)/%.pic.o : %.cpp
	@[ -d $(dir $@ ) ] || mkdir -p $(dir $@)
	$(CXX) -c -Wall -D_REENTRANT -fPIC $(CPPOPTS) $(INCS) $< -o $@
	$(if $(DTRACE),cd ./$(BINDIR); $(DTRACE) -G -s ../ofluxprobe.d $(notdir $@);)

$(OFILES): $(AUTOS)

$(SHIMSO): $(SHIMOFILES)
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(CXX) $(LIBDIRS) -shared $(SHIMOFILES) $(LIBS) -o $(SHIMSO) -ldl

ofluxprobe.h: ofluxprobe.d
	$(if $(DTRACE), $(DTRACE) -h -s ofluxprobe.d)

ifneq ($(MAKECMDGOALS),clean)
 -include $(BINDIR)/.*.depend
endif

clean:
	rm -f $(BINDIR)/* $(BINDIR)/.*.depend ofluxprobe.h oflux_vers.cpp
	rm -rf ../../doc/runtime/* 

FORCE:


.PHONY : clean FORCE
