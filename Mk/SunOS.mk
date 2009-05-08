
# Define ARCH_FLAGS for architecture specific compile flags

LIBS += -lsocket
LDFLAGS = -L/usr/lib \
	-L/oanda/system/lib \
	-L/oanda/system/pkg/expat-1.95.8/lib
LD = /usr/ccs/bin/ld 

