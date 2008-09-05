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
	$(BINDIR)/OFluxAtomicHolder.o \
	$(BINDIR)/OFluxEvent.o \
	$(BINDIR)/OFluxRunTimeAbstract.o \
	$(BINDIR)/OFluxRunTime.o \
	$(BINDIR)/OFluxXML.o

SHIMOFILES= $(BINDIR)/OFluxRunTimeAbstract.pic.o $(BINDIR)/OFluxIOShim.pic.o

OFLUXLIB= $(BINDIR)/liboflux.a
SHIMSO= $(BINDIR)/libofshim.so
ARCH=$(shell uname)
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
CPPOPTS= -O3 -g -MMD -MF "$(BINDIR)/.$(notdir $*).d" #-DOFLUX_RT_DEBUG
ifeq ($(ARCH),Linux)
CPPOPTS += -DLINUX
endif
CXX= g++
#CPPOPTS += -DPROFILING 
#CPPOPTS += -DTHREAD_COLLECTION #-DSOLARIS_NATIVE_THREADS

all: $(OFLUXLIB) $(SHIMSO) docs

docs:
	$(if $(DOXYGEN),$(DOXYGEN) oflux.dox, @echo "install doxygen!!")

$(OFLUXLIB): $(OFILES)
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	ar vr $(OFLUXLIB) $(OFILES)

$(BINDIR)/%.o : %.cpp
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(CXX) -c -ftemplate-depth-20 -Wall -D_REENTRANT $(CPPOPTS) $(INCS) $< -o $@

$(BINDIR)/%.pic.o : %.cpp
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(CXX) -c -ftemplate-depth-20 -Wall -D_REENTRANT -fPIC $(CPPOPTS) $(INCS) $< -o $@

$(SHIMSO): $(SHIMOFILES)
	$(CXX) $(LIBDIRS) -shared $(SHIMOFILES) $(LIBS) -o $(SHIMSO) -ldl

ifneq ($(MAKECMDGOALS),clean)
 -include $(BINDIR)/.*.d
endif

clean:
	rm -f $(BINDIR)/* $(BINDIR)/.*.d 
	rm -rf ../../doc/runtime/*
