#include "OFluxRunTimeAbstractForShim.h"

namespace oflux {

ManagerLock::ManagerLock()
{
	oflux_mutex_init(&_manager_lock);
}

ManagerLock::~ManagerLock()
{
	oflux_mutex_destroy(&_manager_lock);
}

RunTimeAbstractForShim::RunTimeAbstractForShim()
	: _waiting_to_run(&_manager_lock)
	, _waiting_in_pool(&_manager_lock)
{
}

RunTimeAbstractForShim::~RunTimeAbstractForShim()
{
}

UnlockRunTime::UnlockRunTime(RunTimeAbstractForShim * rt)
        : _aul(&(rt->_manager_lock))
        , rt_(rt)
        , prev_detached_(rt->thread()->is_detached())
{ 
rt_->thread()->set_detached(true); 
}

UnlockRunTime::~UnlockRunTime(void)
{ 
        if(rt_ && rt_->running()) {
                rt_->thread()->set_detached(prev_detached_); 
        } else {
                //throw RunTimeAbort();
        }
}

};

