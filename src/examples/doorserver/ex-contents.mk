$(info Reading ex-contents.mk $(COMPONENT_DIR))


OFLUX_PROJECT_NAME:=doorserver
include $(SRCDIR)/Mk/oflux_example.mk

doorclient.o : CXXFLAGS+= $(doorserver_OFLUX_INCS)

doorclient.o : OFluxGenerate_doorserver.h

doorclient : doorclient.o liboflux.so
	$(CXX) $(CXXOPTS) $(doorserver_OFLUX_CXXFLAGS) $(INCS) doorclient.o $(LIBDIRS) liboflux.so libofshim.so $(LIBS) -o $@

doorserver_load_test : doorclient
