
# Define ARCH_FLAGS for architecture specific compile flags

LIBS += -lsocket -lrt
LD = /usr/ccs/bin/ld 

#EXTRA_RUN_ENVIRONMENT="export UMEM_DEBUG=default\nexport UMEM_LOGGING=transaction\nexport LD_PRELOAD=libumem.so.1"
EXTRA_RUN_ENVIRONMENT="export LD_PRELOAD=libumem.so.1"
