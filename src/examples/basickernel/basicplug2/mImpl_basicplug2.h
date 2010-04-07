#ifndef MIMPL_BASICPLUG2
#define MIMPL_BASICPLUG2

#include <cassert>
#include <cstdlib>

class B2 {
public:
	B2() : _b(NULL) {}
	B2(const B2 & b)
		: _b(b._b)
	{ assert(_b == NULL); }
	B2 & operator=(const B2 & b)
	{ _b = b._b; assert(_b == NULL); return *this; }
private:
	int * _b;
};

#endif
