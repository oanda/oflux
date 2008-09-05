#include "OFluxRunTimeAbstract.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <ucontext.h>
#include <dlfcn.h> // dlsym
#include <vector>
#include <sys/poll.h>

#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <queue>
#include <stack>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#ifdef LINUX
#include <sys/epoll.h>
#else
#include <sys/port.h>
#endif
#ifdef HAS_DTRACE
#include "ofluxprobe.h"
#else
#define OFLUX_SHIM_CALL(X)
#define OFLUX_SHIM_WAIT(X)
#define OFLUX_SHIM_RETURN(X)
#endif

#define SHIM_CALL(X) OFLUX_SHIM_CALL(const_cast<char *>(X))
#define SHIM_WAIT(X) OFLUX_SHIM_WAIT(const_cast<char *>(X))
#define SHIM_RETURN(X) OFLUX_SHIM_RETURN(const_cast<char *>(X))

/**
 * @file OFluxIOShim.cpp
 * @author Mark Pichora
 * fairness: this code is largely based on the original Flux shim
 */

template<typename T, size_t S>
class SafeArray
{
public:
    T & operator[](size_t index) { assert(index < S); return buf_[index]; }

private:
    T buf_[S];
};

typedef size_t socklen_t;

oflux::RunTimeAbstract *eminfo = NULL;

SafeArray<bool, 16384> is_regular;

typedef ssize_t (*readFnType) (int, void *, size_t);
typedef ssize_t (*writeFnType) (int, const void*, size_t);
typedef int (*openFnType) (const char*, int);
typedef int (*closeFnType) (int fd);
typedef unsigned int (*sleepFnType) (unsigned int);
typedef int (*usleepFnType) (useconds_t);
typedef int (*acceptFnType) (int, struct sockaddr *, socklen_t *);
typedef int (*selectFnType) (int, fd_set *, fd_set *, fd_set *, struct timeval *);
typedef ssize_t (*recvFnType) (int, void *, size_t, int);
typedef ssize_t (*sendFnType) (int, const void *, size_t, int);
typedef int (*gethostbyname_rFnType) (const char *, struct hostent *, char *, size_t, struct hostent **, int *);
#ifdef LINUX
typedef int (*epoll_waitFnType) (int, struct epoll_event *, int, int);
#else
typedef int (*port_getFnType) (int, port_event_t *, const timespec_t *);
typedef int (*port_getnFnType) (int port, port_event_t [],  uint_t, uint_t *, const timespec_t *);
#endif

static readFnType shim_read = NULL;
static closeFnType shim_close = NULL;
static writeFnType shim_write = NULL;
static openFnType shim_open = NULL;
static sleepFnType shim_sleep = NULL;
static usleepFnType shim_usleep = NULL;
static acceptFnType shim_accept = NULL;
static selectFnType shim_select = NULL;
static recvFnType shim_recv = NULL;
static sendFnType shim_send = NULL;
static gethostbyname_rFnType shim_gethostbyname_r = NULL;
#ifdef LINUX
static epoll_waitFnType shim_epoll_wait = NULL;
#else
static port_getFnType shim_port_get = NULL;
static port_getnFnType shim_port_getn = NULL;
#endif

static int page_size;

extern "C" void initShim(oflux::RunTimeAbstract *eventmgrinfo)
{
	eminfo = eventmgrinfo;

	page_size = getpagesize();

	shim_read = (readFnType)dlsym (RTLD_NEXT, "read");
	shim_open = (openFnType)dlsym (RTLD_NEXT, "open");
	shim_write = (writeFnType)dlsym (RTLD_NEXT, "write");
	shim_close = (closeFnType) dlsym (RTLD_NEXT, "close");
	shim_sleep = (sleepFnType) dlsym(RTLD_NEXT, "sleep");
	shim_usleep = (usleepFnType) dlsym(RTLD_NEXT, "usleep");
	shim_accept = (acceptFnType) dlsym(RTLD_NEXT, "accept");
	shim_select = (selectFnType) dlsym(RTLD_NEXT, "select");
	shim_recv = (recvFnType) dlsym(RTLD_NEXT, "recv");
	shim_send = (sendFnType) dlsym(RTLD_NEXT, "send");
	shim_gethostbyname_r = (gethostbyname_rFnType) dlsym(RTLD_NEXT, "gethostbyname_r");
#ifdef LINUX
	shim_epoll_wait = (epoll_waitFnType) dlsym(RTLD_NEXT, "epoll_wait");
#else
	shim_port_get = (port_getFnType) dlsym(RTLD_NEXT, "port_get");
    shim_port_getn = (port_getnFnType) dlsym(RTLD_NEXT, "port_getn");
#endif
}


extern "C" unsigned int sleep(unsigned int seconds)
{
	if (!eminfo || eminfo->thread()->is_detached()) {
		if (!shim_sleep) {
			shim_sleep = (unsigned int (*)(unsigned int)) dlsym(RTLD_NEXT, "sleep");
		}
		return ((shim_sleep)(seconds));
	}

	unsigned int ret;
	eminfo->wake_another_thread();

	oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
	SHIM_CALL("sleep");
	{
		oflux::UnlockRunTime urt(eminfo);
		ret = ((shim_sleep)(seconds));
	}

	wtr_raii.state_wtr();
	SHIM_WAIT("sleep");
	eminfo->wait_to_run();
	SHIM_RETURN("sleep");

	return ret;
}

extern "C" int usleep(useconds_t useconds)
{
	if (!eminfo || eminfo->thread()->is_detached()) {
		if (!shim_sleep) {
			shim_usleep = (int (*)(useconds_t)) dlsym(RTLD_NEXT, "usleep");
		}
		return ((shim_usleep)(useconds));
	}

	unsigned int ret;
	oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
	eminfo->wake_another_thread();

	SHIM_CALL("usleep");
	{
		oflux::UnlockRunTime urt(eminfo);
		ret = ((shim_usleep)(useconds));
	}

	wtr_raii.state_wtr();
	SHIM_WAIT("usleep");
	eminfo->wait_to_run();
	SHIM_RETURN("usleep");
	return ret;
}

extern "C" int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	if (!eminfo || eminfo->thread()->is_detached()) {
		if (!shim_accept) {
			shim_accept = (int (*) (int, struct sockaddr *, socklen_t *)) dlsym(RTLD_NEXT, "accept");
		}
		return ((shim_accept)(s, addr, addrlen));
	}

	struct pollfd pfd;
	pfd.fd = s;
	pfd.events = POLLIN;
	poll(&pfd, 1, 0);
	if ((pfd.revents & POLLIN) != 0)
		return ((shim_accept)(s, addr, addrlen));

	int ret;
    int mgr_awake = eminfo->wake_another_thread();
	oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
	SHIM_CALL("accept");
	{ 
		oflux::UnlockRunTime urt(eminfo);
		ret = ((shim_accept)(s, addr, addrlen));
	}

	SHIM_WAIT("accept");
	if (mgr_awake == 0) {
		wtr_raii.state_wtr();
		eminfo->wait_to_run();
	}
	SHIM_RETURN("accept");

	return ret;
}

extern "C" ssize_t read(int fd, void *buf, size_t count)
{
	ssize_t ret;

	if (!eminfo || eminfo->thread()->is_detached()) {
		if (!shim_read)  {
			shim_read = (ssize_t (*)(int, void *, size_t))dlsym (RTLD_NEXT, "read");
		}
		return ((shim_read)(fd, buf, count));
	}

    if (!is_regular[fd]) {
		struct pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		poll(&pfd, 1, 0);

		if (pfd.revents & POLLIN) { // if it won't block
			return ((shim_read)(fd, buf, count));
		}
	} else {
		/*
			Use mmap and mincore to find out if the page is in memory,
			if YES, then just do the read
			if NO, then use the thread-pool to fake asynch IO
		*/
		off_t off = lseek(fd, 0, SEEK_CUR);
		void *addr = mmap(0, count, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, off);
		
		int len = (count+page_size-1) / page_size;
		unsigned char *vec = new unsigned char[len];
		//int rt = -- unused
#ifdef LINUX
		mincore((char*)addr, count, (unsigned char*)vec);
#else
		mincore((char*)addr, count, (char*)vec);
#endif
		munmap(addr, count);
		bool in_memory = true;
		
		for (int i=0; i< len; i++) {
			if (vec[i] == 0) {
				in_memory = false;
				break;
			}
		}
		
		delete [] vec;
		
		if (in_memory) {
			return ((shim_read)(fd, buf, count));
		}
	}

	int mgr_awake = eminfo->wake_another_thread();
	oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
	SHIM_CALL("read");
	{
		oflux::UnlockRunTime urt(eminfo);
		ret = ((shim_read)(fd, buf, count));
	}
	SHIM_WAIT("read");
	if (mgr_awake == 0) {
		wtr_raii.state_wtr();
		eminfo->wait_to_run();
	}
	SHIM_RETURN("read");

	return ret;
}

extern "C" int open(const char *pathname, int flags)
{
    if (!eminfo) {
		if (!shim_open) {
			shim_open = (int (*)(const char*, int))dlsym (RTLD_NEXT, "open");
		}
		return ((shim_open)(pathname, flags));
	}
	
	
	int fd = ((*shim_open)(pathname, flags));
	if(-1 == fd) {
	    return fd;
	}
    is_regular[fd] = true;
	
	if (strcmp("/proc/cpuinfo", pathname) == 0) {
		is_regular[fd] = false;
	}
	
	return fd;
}

extern "C" int close(int fd)
{
	if (!eminfo) {
		if (!shim_close) {
			shim_close = (int (*)(int)) dlsym (RTLD_NEXT, "close");
		}
		return ((shim_close)(fd));
	}
	
    is_regular[fd] = false;
	return ((*shim_close)(fd));
}

extern "C" ssize_t write(int fd, const void *buf, size_t count) {
	if (!eminfo || eminfo->thread()->is_detached()) {
		if (!shim_write) {
			shim_write = (writeFnType) dlsym (RTLD_NEXT, "write");
		}
		return ((shim_write)(fd, buf, count));
	}

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLOUT;
	poll(&pfd, 1, 0);

	if (pfd.revents & POLLOUT) { // if it won't block
		return ((shim_write)(fd, buf, count));
	}

	int ret;
	int mgr_awake = eminfo->wake_another_thread();

	oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
	SHIM_CALL("write");
	{
		oflux::UnlockRunTime urt(eminfo);
		ret = ((shim_write)(fd, buf, count));
	}
	SHIM_WAIT("write");
	if (mgr_awake == 0) {
		wtr_raii.state_wtr();
		eminfo->wait_to_run();
	}
	SHIM_RETURN("write");

	return ret;
}

extern "C" int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
	if (!eminfo || eminfo->thread()->is_detached()) {
		if (!shim_select) {
			shim_select = (selectFnType) dlsym (RTLD_NEXT, "select");
		}
		return ((shim_select)(n, readfds, writefds, exceptfds, timeout));
	}

	int ret;
	int mgr_awake = eminfo->wake_another_thread();

	oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
	SHIM_CALL("select");
	{
		oflux::UnlockRunTime urt(eminfo);
		ret = ((shim_select)(n, readfds, writefds, exceptfds, timeout));
	}
	SHIM_WAIT("select");
	if (mgr_awake == 0) {
		wtr_raii.state_wtr();
		eminfo->wait_to_run();
	}
	SHIM_RETURN("select");
	return ret;
}

extern "C" ssize_t recv(int s, void * buf, size_t len, int flags) {
    ssize_t ret;

    if (!eminfo || eminfo->thread()->is_detached()) {
        if (!shim_recv)  {
            shim_recv = (recvFnType) dlsym (RTLD_NEXT, "recv");
        }
        return ((shim_recv)(s, buf, len, flags));
    }

    struct pollfd pfd;
    pfd.fd = s;
    pfd.events = POLLIN;
    poll(&pfd, 1, 0);

    if (pfd.revents & POLLIN || flags & MSG_DONTWAIT) { // if it won't block
        return ((shim_recv)(s, buf, len, flags));
    }
    
    int mgr_awake = eminfo->wake_another_thread();
    oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
    SHIM_CALL("recv");
    {
        oflux::UnlockRunTime urt(eminfo);
        ret = ((shim_recv)(s, buf, len, flags));
    }
    SHIM_WAIT("recv");
    if (mgr_awake == 0) {
        wtr_raii.state_wtr();
        eminfo->wait_to_run();
    }
    SHIM_RETURN("recv");

    return ret;
}

extern "C" ssize_t send(int s, const void * msg, size_t len, int flags) {
    if (!eminfo || eminfo->thread()->is_detached()) {
        if (!shim_send) {
            shim_send = (sendFnType) dlsym (RTLD_NEXT, "send");
        }
        return ((shim_send)(s, msg, len, flags));
    }
    
    struct pollfd pfd;
    pfd.fd = s;
    pfd.events = POLLOUT;
    poll(&pfd, 1, 0);

    if (pfd.revents & POLLOUT) { // if it won't block
        return ((shim_send)(s, msg, len, flags));
    }

    ssize_t ret;
    int mgr_awake = eminfo->wake_another_thread();

    oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
    SHIM_CALL("send");
    {
        oflux::UnlockRunTime urt(eminfo);
        ret = ((shim_send)(s, msg, len, flags));
    }
    SHIM_WAIT("send");
    if (mgr_awake == 0) {
	wtr_raii.state_wtr();
        eminfo->wait_to_run();
    }
    SHIM_RETURN("send");

    return ret;
}

extern "C" int gethostbyname_r(const char *name,
                               struct hostent *ret, char *buf, size_t buflen,
                               struct hostent **result, int *h_errnop)
{
    if (!eminfo || eminfo->thread()->is_detached()) {
        if (!shim_gethostbyname_r) {
            shim_gethostbyname_r = (gethostbyname_rFnType) dlsym (RTLD_NEXT, "gethostbyname_r");
        }
        return ((shim_gethostbyname_r)(name, ret, buf, buflen, result, h_errnop));
    }

    int retval;
    int mgr_awake = eminfo->wake_another_thread();

    oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
    SHIM_CALL("gethostbyname_r");
    {
        oflux::UnlockRunTime urt(eminfo);
        retval = ((shim_gethostbyname_r)(name, ret, buf, buflen, result, h_errnop));
    }
    SHIM_WAIT("gethostbyname_r");
    if (mgr_awake == 0) {
        wtr_raii.state_wtr();
        eminfo->wait_to_run();
    }
    SHIM_RETURN("gethostbyname_r");
    return retval;
}

#ifdef LINUX
extern "C" int epoll_wait(int epfd, struct epoll_event * events,
                          int maxevents, int timeout)
{
    if (!eminfo || eminfo->thread()->is_detached()) {
        if (!shim_epoll_wait) {
            shim_epoll_wait = (epoll_waitFnType) dlsym (RTLD_NEXT, "epoll_wait");
        }
        return ((shim_epoll_wait)(epfd, events, maxevents, timeout));
    }

    int ret;
    int mgr_awake = eminfo->wake_another_thread();

    oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
    SHIM_CALL("epoll_wait");
    {
        oflux::UnlockRunTime urt(eminfo);
        ret = ((shim_epoll_wait)(epfd, events, maxevents, timeout));
    }
    SHIM_WAIT("epoll_wait");
    if (mgr_awake == 0) {
	wtr_raii.state_wtr();
        eminfo->wait_to_run();
    }
    SHIM_RETURN("epoll_wait");
    return ret;
}
#endif

#ifndef LINUX
extern "C" int port_get(int port, port_event_t * pe, const timespec_t * timeout) {
    if (!eminfo || eminfo->thread()->is_detached()) {
        if (!shim_port_get) {
            shim_port_get = (port_getFnType) dlsym (RTLD_NEXT, "port_get");
        }
        return ((shim_port_get)(port, pe, timeout));
    }
    
    uint_t nget = 0;
    ((shim_port_getn)(port, 0, 0, &nget, 0));
    if(nget > 0) { // if it won't block
        return ((shim_port_get)(port, pe, timeout));
    }

    int ret;
    int mgr_awake = eminfo->wake_another_thread();

    oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
    SHIM_CALL("port_get");
    {
        oflux::UnlockRunTime urt(eminfo);
        ret = ((shim_port_get)(port, pe, timeout));
    }
    SHIM_WAIT("port_get");
    if (mgr_awake == 0) {
	wtr_raii.state_wtr();
        eminfo->wait_to_run();
    }
    SHIM_RETURN("port_get");
    return ret;
}

extern "C" int port_getn(int port, port_event_t list[], uint_t max,
                         uint_t * nget, const timespec_t * timeout) {
    if (!eminfo || eminfo->thread()->is_detached()) {
        if (!shim_port_getn) {
            shim_port_getn = (port_getnFnType) dlsym (RTLD_NEXT, "port_getn");
        }
        return ((shim_port_getn)(port, list, max, nget, timeout));
    }
    
    if(0 == max) { // if it won't block
        return ((shim_port_getn)(port, list, max, nget, timeout));
    }

    int ret;
    int mgr_awake = eminfo->wake_another_thread();

    oflux::WaitingToRunRAII wtr_raii(eminfo->thread());
    SHIM_CALL("port_getn");
    {
        oflux::UnlockRunTime urt(eminfo);
        ret = ((shim_port_getn)(port, list, max, nget, timeout));
    }
    SHIM_WAIT("port_getn");
    if (mgr_awake == 0) {
	wtr_raii.state_wtr();
        eminfo->wait_to_run();
    }
    SHIM_RETURN("port_getn");

    return ret;
}
#endif
