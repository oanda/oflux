# top level make file for this project
#

OFLUX= tools/bin/oflux
RUNTIMES= c++.1 c++.2
EXAMPLES= \
	webserver \
	sleeppipe \
	acctcache \
	guardtest1 \
	guardtest2 \
	moduletest1 \
	concur1 \
	splaytest1 \
	initial1

################################################################
#
EXAMPLESWITHDIRS= $(addprefix examples/,$(EXAMPLES))
RUNTIMESWITHDIRS= $(addprefix runtime/,$(RUNTIMES))

OFLUXRUNTIMES= $(addsuffix /bin/liboflux.a,$(RUNTIMESWITHDIRS))
EXAMPLESBUILT= $(foreach ex,$(EXAMPLES),examples/$(ex)/$(ex).xml) 

all: $(OFLUX) $(OFLUXRUNTIMES)

test:
	@echo "EXAMPLESWITHDIRS=" $(EXAMPLESWITHDIRS)
	@echo "RUNTIMESWITHDIRS=" $(RUNTIMESWITHDIRS)
	@echo "OFLUXRUNTIMES=" $(OFLUXRUNTIMES)
	@echo "EXAMPLESBUILT=" $(EXAMPLESBUILT)

$(OFLUX):
	make -wC tools depend all

$(OFLUXRUNTIMES):
	$(foreach rtd,$(RUNTIMESWITHDIRS),make -wC $(rtd);)

$(EXAMPLESBUILT): $(OFLUX) $(OFLUXRUNTIMES)
	$(foreach exd,$(EXAMPLESWITHDIRS),make -wC $(exd);)

examples: $(EXAMPLESBUILT)

clean:
	make -wC tools clean;
	$(foreach rtd,$(RUNTIMESWITHDIRS),make -wC $(rtd) clean;)
	$(foreach exd,$(EXAMPLESWITHDIRS),make -wC $(exd) clean;)

.PHONY : package
package :
	echo "Debian package not available for oflux.  Executing default target..."
package : all
