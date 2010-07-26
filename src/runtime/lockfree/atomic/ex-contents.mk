$(info Reading ex-contents.mk $(COMPONENT_DIR))

# build tests

test_lfhashtable: liboflux.so test_lfhashtable.cpp $(LIBS)

test_atomic: test_atomic.cpp $(LIBS)

test_pool_atomic_unfair: test_pool_atomic_unfair.cpp $(LIBS)

test_pool_atomic_fair: test_pool_atomic_fair.cpp $(LIBS)

test_rw_atomic: test_rw_atomic.cpp $(LIBS)

test_rw_atomic2: test_rw_atomic2.cpp $(LIBS) liboflux.so libofshim.so
