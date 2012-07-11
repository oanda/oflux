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

