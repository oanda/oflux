#ifndef _OFLUX_RUNTIME_ABSTRACT_FOR_SHIM
#define _OFLUX_RUNTIME_ABSTRACT_FOR_SHIM

/**
 * @file OFluxRunTimeAbstract.h
 * @author Mark Pichora
 * This contains the minimum that the SHIM needs to access the run time.
 */

#include "OFlux.h"
#include "OFluxRunTimeAbstract.h"
#include "OFluxRunTimeThreadAbstract.h"
#include "OFluxProfiling.h"
#include "OFluxWrappers.h"
#include "OFluxWatermark.h"


namespace oflux {

/**
 * @class ManagerLock
 * @brief container for the mutex
 */
class ManagerLock {
public:
	ManagerLock();
	~ManagerLock();
public: // sorry C++ ppl
	oflux_mutex_t  _manager_lock;
};

typedef RunTimeThreadAbstract RunTimeThreadAbstractForShim;


/**
 * @class RunTimeAbstract
 * @brief base class with functionality to punt control to the run time
 */
class RunTimeAbstractForShim : public ManagerLock, public RunTimeAbstract {
public:
	RunTimeAbstractForShim();
	virtual ~RunTimeAbstractForShim();

	/**
	 * @brief wait in the "waiting to run" queue (thread conditional wait)
	 **/
	inline void wait_to_run() 
	{
		_waiting_to_run.wait();
	}

	/**
	 * @brief wait in the "waiting in pool" queue (thread conditional wait)
	 **/
	inline void wait_in_pool() 
	{
		_waiting_in_pool.wait();
	}

        virtual bool running() = 0;

	/**
	 * @brief allow another thread to come up and process events
	 *
	 **/
	virtual int wake_another_thread() = 0;

	virtual RunTimeThreadAbstract * thread() = 0;

	virtual bool currently_detached() = 0;
public: // sorry C++ ppl
	ConditionVariable<IntegerCounter>     _waiting_to_run;
#ifdef THREAD_COLLECTION
	ConditionVariable<WatermarkedCounter> 
#else
	ConditionVariable<IntegerCounter> 
#endif
		_waiting_in_pool;
};


class WaitingToRunRAII {
public:
	WaitingToRunRAII(RunTimeThreadAbstractForShim * rtt)
		: _rtt(rtt)
#ifdef PROFILING
		, _tp(rtt->timer_list())
#endif
	{ _rtt->wait_state(RTTWS_blockingcall); }
	~WaitingToRunRAII()
	{ _rtt->wait_state(RTTWS_running); }
	inline void state_wtr() {  _rtt->wait_state(RTTWS_wtrshim); }
private:
	RunTimeThreadAbstractForShim * _rtt;
#ifdef PROFILING
	TimerPause       _tp;
#endif
};


/**
 * @brief an RAII class for unlocking a runtime
 *
 */
class UnlockRunTime {
public:
	UnlockRunTime(RunTimeAbstractForShim * rt);
        
	~UnlockRunTime();
private:
	AutoUnLock _aul;
	RunTimeAbstractForShim *rt_;
	bool prev_detached_;
};



};


#endif // _OFLUX_RUNTIME_ABSTRACT
