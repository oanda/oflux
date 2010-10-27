$(info Reading ex-contents.mk $(COMPONENT_DIR))


OFLUX_PROJECT_NAME:=doorserver
include $(SRCDIR)/Mk/oflux_example.mk

doorclient.o : CXXFLAGS+= $(doorserver_OFLUX_INCS)

doorclient : doorclient.o liboflux.so
	$(CXX) $(CXXOPTS) $(doorserver_OFLUX_CXXFLAGS) $(INCS) $(LIBDIRS) $(doorserver_OFLUX_MAIN_OBJ_DEP) $(doorserver_OFLUX_KERNEL:%.cpp=-l%) liboflux.so libofshim.so $(LIBS) -o $@
