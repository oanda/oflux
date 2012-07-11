#ifndef _OFLUX_THREADS
#define _OFLUX_THREADS
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file OFluxThreads.h
 * @author Mark Pichora
 * @brief capture the common API for what ever thread libraries are supported
 */

#ifdef SOLARIS_NATIVE_THREADS
# include <thread.h>
namespace oflux {
  typedef mutex_t oflux_mutex_t;
  typedef cond_t oflux_cond_t;
  typedef thread_t oflux_thread_t;
  typedef thread_key_t oflux_key_t;
  typedef rwlock_t oflux_rwlock_t;
} // namespace
# define oflux_mutex_init(X) mutex_init(X,USYNC_THREAD,0)
# define oflux_mutex_destroy(X) mutex_destroy(X)
# define oflux_mutex_lock(X) mutex_lock(X)
# define oflux_mutex_unlock(X) mutex_unlock(X)
# define oflux_rwlock_init(X) rwlock_init(X,USYNC_THREAD,0)
# define oflux_rwlock_destroy(X) rwlock_destroy(X)
# define oflux_rwlock_rdlock(X) rw_rdlock(X)
# define oflux_rwlock_wrlock(X) rw_wrlock(X)
# define oflux_rwlock_unlock(X) rw_unlock(X)
# define oflux_cond_init(X) cond_init(X,USYNC_THREAD,0)
# define oflux_cond_destroy(X) cond_destroy(X)
# define oflux_self thr_self // function on ()
# define oflux_cond_wait(X,Y) cond_wait(X,Y)
# define oflux_cond_signal(X) cond_signal(X)
# define oflux_key_create(X,Y) thr_keycreate(X,Y)
# define oflux_key_delete(X) 
# define oflux_cancel(X) thr_kill(X,SIGTERM) // somewhat iffy
# define oflux_kill_int(X) thr_kill(X,SIGINT) 
namespace oflux {
  inline void * oflux_get_specific(thread_key_t x) { int * ptr; thr_getspecific(x,(void**) &ptr); return (void*) ptr; }
  inline int oflux_create_thread(size_t stack_size, void *(*start_routine) (void *), void * data_arg, oflux_thread_t * thr) { 
	static const long flags = THR_BOUND | THR_DETACHED;
	return thr_create(        NULL // stack base ptr
				, stack_size // stack size
				, start_routine // run function
				, data_arg
				, flags
				, thr);
  }
} // namespace
# define oflux_set_specific(X,Y) thr_setspecific(X,Y)
#else
# include <pthread.h>
namespace oflux {
  typedef pthread_mutex_t oflux_mutex_t;
  typedef pthread_cond_t oflux_cond_t;
  typedef pthread_t oflux_thread_t;
  typedef pthread_key_t oflux_key_t;
  typedef pthread_rwlock_t oflux_rwlock_t;
} // namespace
# define oflux_mutex_init(X) pthread_mutex_init(X,NULL)
# define oflux_mutex_destroy(X) pthread_mutex_destroy(X)
# define oflux_mutex_lock(X) pthread_mutex_lock(X)
# define oflux_mutex_unlock(X) pthread_mutex_unlock(X)
# define oflux_rwlock_init(X) pthread_rwlock_init(X,NULL)
# define oflux_rwlock_destroy(X) pthread_rwlock_destroy(X)
# define oflux_rwlock_rdlock(X) pthread_rwlock_rdlock(X)
# define oflux_rwlock_wrlock(X) pthread_rwlock_wrlock(X)
# define oflux_rwlock_unlock(X) pthread_rwlock_unlock(X)
# define oflux_cond_init(X) pthread_cond_init(X,NULL)
# define oflux_cond_destroy(X) pthread_cond_destroy(X)
# define oflux_self pthread_self // function on ()
# define oflux_cond_wait(X,Y) pthread_cond_wait(X,Y)
# define oflux_cond_signal(X) pthread_cond_signal(X)
# define oflux_cond_broadcast(X) pthread_cond_broadcast(X)
# define oflux_testcancel() pthread_testcancel()
# define oflux_key_create(X,Y) pthread_key_create(X,Y)
# define oflux_key_delete(X) pthread_key_delete(X)
# define oflux_get_specific(X) pthread_getspecific(X)
# define oflux_set_specific(X,Y) pthread_setspecific(X,Y)
# define oflux_cancel(X) pthread_cancel(X)
# define oflux_join(X,Y) pthread_join(X,Y)
# define oflux_kill_int(X) pthread_kill(X,SIGINT)
namespace oflux {
  inline int oflux_create_thread(size_t stack_size, void *(*start_routine) (void *), void * data_arg, oflux_thread_t * thr) { 
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stack_size);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int rc = pthread_create(thr, &attr, start_routine, data_arg);
	if(!rc) pthread_detach(*thr);
	return rc;
  }
}
#endif
#ifdef LINUX
# define PTHREAD_PRINTF_FORMAT "%lu"
#else
# define PTHREAD_PRINTF_FORMAT "%x"
#endif

#endif // _OFLUX_THREADS
