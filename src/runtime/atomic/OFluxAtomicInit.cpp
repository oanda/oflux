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
#include "atomic/OFluxAtomic.h"
#include "atomic/OFluxAtomicInit.h"
#include <vector>
#include <cassert>

namespace oflux {
namespace atomic {

GuardInserter::~GuardInserter()
{
        std::vector<EventBasePtr> vec;
	EventBasePtr nullPtr(NULL);
        for(int i = 0; i < (int)_to_release.size(); i++) {
                _to_release[i]->release(vec,nullPtr);
        }
        assert(vec.empty());
}

bool GuardInserter::insert(const void * key, void * value)
{
	bool res = true;
	Atomic * aptr;
	_ama->get(aptr, key);
	assert(aptr);
	void ** d = aptr->data();
	if(*d == NULL) {
		*d = value;
	} else {
		res = false;
	}
        if(aptr->is_pool_like()) {
		_to_release.push_back(aptr);
	}
	return res;
}

void * GuardInserter::swap(const void * key, void * newvalue)
{
	void * oldvalue = NULL;
	Atomic * aptr;
	_ama->get(aptr, key);
	assert(aptr);
	void ** d = aptr->data();
	oldvalue = *d;
	*d = newvalue;
        if(aptr->is_pool_like()) {
		_to_release.push_back(aptr);
	}
	return oldvalue;
}

void * GuardInserter::get(const void * key)
{
	Atomic * aptr;
	_ama->get(aptr, key);
	assert(aptr);
	void ** d = aptr->data();
	return *d;
}

GuardWalker::GuardWalker(AtomicMapAbstract *ama)
        : _walker(ama->walker())
{
}

GuardWalker::~GuardWalker()
{
        delete _walker;
}

bool GuardWalker::next(const void *& key, void ** & value)
{
        Atomic * atom = NULL;
        bool res = _walker->next(key,atom);
        if(res) value = atom->data();
        return res;
}

} // namespace atomic
} // namespace oflux
