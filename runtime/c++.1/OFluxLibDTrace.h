#ifndef OFLUX_LIB_DTRACE_H
#define OFLUX_LIB_DTRACE_H

//
// This header file must be invisible to OFluxGenerate_...
//

#ifdef HAS_DTRACE

# include "ofluxprobe.h"

#else // ! HAS_DTRACE

# define OFLUX_GUARD_ACQUIRE(X,Y,Z)
# define OFLUX_GUARD_WAIT(X,Y,Z)
# define OFLUX_GUARD_RELEASE(X,Y,Z)

# define OFLUX_FIFO_PUSH(X)
# define OFLUX_FIFO_POP(X)

# define OFLUX_NODE_START(X,Y,Z)
# define OFLUX_NODE_DONE(X)
# define OFLUX_NODE_HAVEALLGUARDS(X)
# define OFLUX_NODE_ACQUIREGUARDS(X)

#endif // HAS_DTRACE

#define _NODE_START(X,Y,Z) OFLUX_NODE_START(const_cast<char *>(X),Y,Z)
#define _NODE_DONE(X)      OFLUX_NODE_DONE(const_cast<char *>(X))
#define _NODE_HAVEALLGUARDS(X) \
     OFLUX_NODE_HAVEALLGUARDS(const_cast<char *>(X))
#define _NODE_ACQUIREGUARDS(X) \
     OFLUX_NODE_ACQUIREGUARDS(const_cast<char *>(X))


#define _GUARD_ACQUIRE(X,Y,Z) OFLUX_GUARD_ACQUIRE(const_cast<char *>(X),const_cast<char *>(Y), Z)
#define _GUARD_WAIT(X,Y,Z)    OFLUX_GUARD_WAIT(const_cast<char *>(X),const_cast<char *>(Y), Z)
#define _GUARD_RELEASE(X,Y,Z) OFLUX_GUARD_RELEASE(const_cast<char *>(X),const_cast<char *>(Y), Z)


#define _FIFO_PUSH(X) OFLUX_FIFO_PUSH(const_cast<char *>(X))
#define _FIFO_POP(X)  OFLUX_FIFO_POP(const_cast<char *>(X))

#endif
