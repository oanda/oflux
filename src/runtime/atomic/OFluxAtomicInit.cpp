#include "atomic/OFluxAtomic.h"
#include "atomic/OFluxAtomicInit.h"
#include <vector>
#include <cassert>

namespace oflux {
namespace atomic {

GuardInserter::~GuardInserter()
{
        std::vector<EventBasePtr> vec;
	EventBasePtr nullPtr;
        for(int i = 0; i < (int)_to_release.size(); i++) {
                _to_release[i]->release(vec,nullPtr);
        }
        assert(vec.size() == 0);
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
        if(aptr->is_pool_like_init()) {
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
        if(aptr->is_pool_like_init()) {
		_to_release.push_back(aptr);
	}
	return oldvalue;
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
