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

# define OFLUX_FIFO_PUSH(E,X)
# define OFLUX_FIFO_POP(E,X)

# define OFLUX_NODE_START(E,X,Y,Z)
# define OFLUX_NODE_DONE(E,X)
# define OFLUX_NODE_HAVEALLGUARDS(E,X)
# define OFLUX_NODE_ACQUIREGUARDS(E,X)

# define OFLUX_EVENT_BORN(E,X)
# define OFLUX_EVENT_DEATH(E,X)

# define OFLUX_AHA_EXCEPTION_BEGIN_THROW(A)

#endif // HAS_DTRACE

#define _EVENT_BORN(E,X) \
    OFLUX_EVENT_BORN(const_cast<void *>(E),const_cast<char *>(X)
#define _EVENT_DEATH(E,X) \
    OFLUX_EVENT_DEATH(const_cast<void *>(E),const_cast<char *>(X)

#define _NODE_START(E,X,Y,Z) \
    OFLUX_NODE_START(const_cast<void *>(E),const_cast<char *>(X),Y,Z)
#define _NODE_DONE(E,X) \
    OFLUX_NODE_DONE(const_cast<void *>(E),const_cast<char *>(X))
#define _NODE_HAVEALLGUARDS(E,X) \
    OFLUX_NODE_HAVEALLGUARDS(const_cast<void *>(E),const_cast<char *>(X))
#define _NODE_ACQUIREGUARDS(E,X) \
    OFLUX_NODE_ACQUIREGUARDS(static_cast<void *>(E),const_cast<char *>(X))


#define _GUARD_ACQUIRE(X,Y,Z) \
    OFLUX_GUARD_ACQUIRE(const_cast<char *>(X), \
        const_cast<char *>(Y), Z)
#define _GUARD_WAIT(X,Y,Z) \
    OFLUX_GUARD_WAIT(const_cast<char *>(X), \
        const_cast<char *>(Y), Z)
#define _GUARD_RELEASE(X,Y,Z) \
    OFLUX_GUARD_RELEASE(const_cast<char *>(X), \
        const_cast<char *>(Y), Z)


#define _FIFO_PUSH(E,Y) \
    OFLUX_FIFO_PUSH(static_cast<void *>(E),const_cast<char *>(Y))
#define _FIFO_POP(E,Y) \
    OFLUX_FIFO_POP(static_cast<void *>(E),const_cast<char *>(Y))


#define _AHA_EXCEPTION_BEGIN_THROW(A) \
    OFLUX_AHA_EXCEPTION_BEGIN_THROW(A)

#endif
