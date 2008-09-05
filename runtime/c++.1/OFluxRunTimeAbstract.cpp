#include "OFluxRunTimeAbstract.h"

namespace oflux {

ManagerLock::ManagerLock()
{
	oflux_mutex_init(&_manager_lock);
}

ManagerLock::~ManagerLock()
{
	oflux_mutex_destroy(&_manager_lock);
}

RunTimeAbstract::RunTimeAbstract()
	: _waiting_to_run(&_manager_lock)
	, _waiting_in_pool(&_manager_lock)
{
}

RunTimeAbstract::~RunTimeAbstract()
{
}

};

