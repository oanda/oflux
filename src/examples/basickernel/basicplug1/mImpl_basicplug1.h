#ifndef MIMPL_BASICPLUG1
#define MIMPL_BASICPLUG1

#include <cassert>
#include <cstdlib>

class B1 {
public:
	B1() : _b(NULL) {}
	B1(const B1 & b)
		: _b(b._b)
	{ assert(_b == NULL); }
	B1 & operator=(const B1 & b)
	{ _b = b._b; assert(_b == NULL); return *this; }
private:
	int * _b;
};

#endif
