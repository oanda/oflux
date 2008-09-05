#include "OFluxAtomic.h"
#include "OFluxAtomicInit.h"
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
	return res;
}

} // namespace
