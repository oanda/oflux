$(warn Reading rules.mk)

CC := gcc 
#
# Be paranoid about building archives and delete any prior versions
# before building new versions.  Not doing this can lead to archives
# containing stale code under some circumstances.
#
%.a:
	$(RM) $@
	$(AR) -q $@ $^

%.gch: $(HEADERS)  
	$(CXX) $(CPPFLAGS) $^ -o $@

CPPFLAGS += $(BASECXXFLAGS) $(COMPONENT_FLAGS) $(ARCH_FLAGS) $(INCS) -D$(_ARCH) 
BOOSTDIR := /oanda/system/include/boost-1_34_1

BASECXXFLAGS = -Wall -D_REENTRANT -ggdb -O0 -MMD -MF "$(notdir $*).d"

INCS = \
    -I/oanda/system/include \
    -I/oanda/system/include/boost-1_34_1 \
    -I$(BOOSTDIR) \
    -I$(SRCDIR)/private_include \
    -I$(SRCDIR)/include \
    -I$(SRCDIR)/src \
    -I$(OFLUX_RUNTIME)


# Include any specific settings for this RELEASE (eg. production, debug, etc)
-include $(SRCDIR)/Mk/$(RELEASE).mk

# Include any specific settings for this architecture (eg. Linux, Sparc, etc)
-include $(SRCDIR)/Mk/$(_ARCH).mk

# Used by libofshim 
%.so: 
	$(CXX) $(CPPFLAGS) -shared -o $@ $(COMMONOBJS) $<

%.pic.o: %.cpp
	$(CXX) $(CPPFLAGS) -c -fPIC $(CXXFLAGS) $< -o $@

#OFLUX
OFLUXCOMPILER := $(CURDIR)/oflux

# OCAML
OCAMLCOMPILER:=ocamlopt $(if $(findstring profile,$(OCAMLCONFIG)),-p,)
OBJECTEXT:=cmx
LIBRARYEXT:=cmxa


%.cmi: %.mli 
	$(OCAMLCOMPILER) $(INCLUDEOPTS) -o $@ -c $<

%.cmx: %.ml 
	$(OCAMLCOMPILER) $(INCLUDEOPTS) -o $@ -c $<




