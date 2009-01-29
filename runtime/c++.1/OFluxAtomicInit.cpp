#include "OFluxAtomic.h"
#include "OFluxAtomicInit.h"
#include <vector>
#include <cassert>

namespace oflux {

GuardInserter::~GuardInserter()
{
        std::vector<boost::shared_ptr<EventBase> > vec;
        for(int i = 0; i < (int)_to_release.size(); i++) {
                _to_release[i]->release(vec);
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
        _to_release.push_back(aptr);
	return res;
}

GuardWalker::GuardWalker(AtomicMapAbstract *ama)
        : _walker(ama->walker())
{
}

GuardWalker::~GuardWalker()
{
        delete _walker;
}

bool GuardWalker::next(const void *& key, void *& value)
{
        Atomic * atom = NULL;
        bool res = _walker->next(key,atom);
        if(res) value = *(atom->data());
        return res;
}

} // namespace
