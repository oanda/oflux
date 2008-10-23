#ifndef _OFLUX_THREADS
#define _OFLUX_THREADS

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
# define oflux_key_create(X,Y) pthread_key_create(X,Y)
# define oflux_key_delete(X) pthread_key_delete(X)
# define oflux_get_specific(X) pthread_getspecific(X)
# define oflux_set_specific(X,Y) pthread_setspecific(X,Y)
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

#endif // _OFLUX_THREADS
