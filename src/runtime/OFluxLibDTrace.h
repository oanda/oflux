#ifndef OFLUX_LIB_DTRACE_H
#define OFLUX_LIB_DTRACE_H

// this header is safe to consume everywhere (now)


namespace oflux {

#ifdef HAS_DTRACE

void PUBLIC_GUARD_ACQUIRE(const char *,const char *,int);
void PUBLIC_GUARD_WAIT(const char *,const char *,int);
void PUBLIC_GUARD_RELEASE(const char *,const char *,int);

void PUBLIC_FIFO_PUSH(const void *,const char *);
void PUBLIC_FIFO_POP(const void *,const char *);

void PUBLIC_NODE_START(const void *,const char * X,int Y,int Z);
void PUBLIC_NODE_DONE(const void *,const char * X);
void PUBLIC_NODE_HAVEALLGUARDS(const void *, const char *);
void PUBLIC_NODE_ACQUIREGUARDS(const void *, const char *);

void PUBLIC_EVENT_BORN(const void *,const char * X);
void PUBLIC_EVENT_DEATH(const void *,const char * X);

void PUBLIC_AHA_EXCEPTION_BEGIN_THROW(int);

#else // ! HAS_DTRACE

# define PUBLIC_GUARD_ACQUIRE(X,Y,Z)
# define PUBLIC_GUARD_WAIT(X,Y,Z)
# define PUBLIC_GUARD_RELEASE(X,Y,Z)

# define PUBLIC_FIFO_PUSH(E,X)
# define PUBLIC_FIFO_POP(E,X)

# define PUBLIC_NODE_START(E,X,Y,Z)
# define PUBLIC_NODE_DONE(E,X)
# define PUBLIC_NODE_HAVEALLGUARDS(E,X)
# define PUBLIC_NODE_ACQUIREGUARDS(E,X)

# define PUBLIC_EVENT_BORN(E,X)
# define PUBLIC_EVENT_DEATH(E,X)

# define PUBLIC_AHA_EXCEPTION_BEGIN_THROW(A)

#endif // HAS_DTRACE


} // namespace oflux

#endif  // OFLUX_LIB_DTRACE_H
