#include "OFluxAtomic.h"
#include "OFluxAtomicInit.h"
#include <vector>
#include <cassert>

namespace oflux {

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
        std::vector<boost::shared_ptr<EventBase> > vec;
        aptr->release(vec);
        assert(vec.size() == 0);
	return res;
}

} // namespace
