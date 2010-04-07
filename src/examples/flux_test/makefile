FLUXSOURCE=$(shell ls *.flux)
FLUXDEFINES= 

ARCH=$(shell uname)

BINDIR=./bin
SYSLIBDIR=/oanda/system/pkg
OFLUXRTDIR=../../runtime/c++.1
FLUXDEFINES+= $(shell strings $(OFLUXRTDIR)/bin/liboflux.a | grep CXXOPTS= | sed -e's/CXXOPTS=//g')
OFLUXRTLIBS=$(OFLUXRTDIR)/bin
RUNTIMELIBDIR=$(OFLUXRTDIR)/bin

SHIMSO= libofshim.so

INCS += \
	-I$(OFLUXRTDIR) \
	-I.

ifeq ($(ARCH),Linux)
LIBS= -loflux -lexpat -lpthread -ldl -lrt
else
LIBS= -loflux -lexpat -lpthread -ldl -lsocket
endif

LIBDIRS= \
	-L$(OFLUXRTLIBS) \
	-L/usr/lib \
	-L/oanda/system/lib \
	-L$(SYSLIBDIR)/expat-1.95.8/lib 

LIBS += -lboost_filesystem
	
DOT=$(shell which dot | sed -e 's/^no .*//g')
FLUX= ../../tools/bin/oflux

BIN=$(basename $(FLUXSOURCE))
TARGET= $(BINDIR)/$(BIN)

LOCALOBJS= \
	bin/OFluxGenerate.o \
	bin/init.o \
	bin/main.o \
	bin/Select.o \
	bin/Read.o \
	bin/Route.o \
	bin/Send.o \
	bin/One.o \
	bin/Two.o \
	bin/Three.o \
	bin/Four.o \
	bin/Five.o \

XMLFILE= $(basename $(FLUXSOURCE)).xml
DOTFILE= $(basename $(FLUXSOURCE)).dot
SVGFILE= $(basename $(FLUXSOURCE)).svg
GENERATEDFILES= OFluxGenerate.cpp OFluxGenerate.h $(XMLFILE) $(DOTFILE)

all: $(GENERATEDFILES) $(TARGET) run.sh

$(SVGFILE): $(DOTFILE)
	$(if $(DOT), $(DOT) -Tsvg $(DOTFILE) > $(SVGFILE), @echo "Install dot")

doc: $(SVGFILE)

ifneq ($(MAKECMDGOALS),clean)
 -include .*.depend
endif

$(TARGET): $(LOCALOBJS)
	$(CXX) $(FLUXDEFINES) $(INCS) $(LIBDIRS) $(LOCALOBJS) $(LIBS) -o $(TARGET) $(GPROF)

$(GENERATEDFILES): $(FLUXSOURCE)
	$(FLUX) $(FLUXSOURCE)

bin/%.o : %.cpp
	$(CXX) $(INCS) $(FLUXDEFINES) -c -ftemplate-depth-20 -Wall -D_REENTRANT $(CXXFLAGS) $< -o $@ $(GPROF)

EXEC = "bin/$(BIN) $(XMLFILE) "
ifdef RATEHOST
EXEC +="--rate-host $(RATEHOST)"
endif
EXEC +="\$$1 \$$2"

run.sh:
	echo "#!$(BASH)" > run.sh
	echo "" >> run.sh
	echo "export UMEM_DEBUG=default" >> run.sh
	echo "export UMEM_LOGGING=transaction" >> run.sh
	echo "export LD_PRELOAD=$(RUNTIMELIBDIR)/$(SHIMSO):libumem.so.1" >> run.sh
ifneq (dev, $(CONFIG))
	echo "export LD_LIBRARY_PATH=\$$LD_LIBRARY_PATH:$(YAMLLIBS)" >> run.sh
endif

	echo "" >> run.sh
	echo $(EXEC) >> run.sh
	chmod +x run.sh

.PHONY : clean VERSION doc
clean:
	-rm -f $(GENERATEDFILES) $(LOCALOBJS) $(TARGET) run.sh .*.depend vers.cpp $(SVGFILE)
