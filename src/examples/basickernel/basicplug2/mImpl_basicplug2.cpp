#include "OFluxGenerate_basickernel_basicplug2.h"
#include <cassert>

namespace basicplug2 {

int
Bar(const Bar_in * in, Bar_out * out, Bar_atoms *)
{
	out->c1 = new char('a' + (char)(in->a %26));
	//out->c2 = NULL;
	out->dc = 0;
	return 0;
}

int
Consume(const Consume_in * in, Consume_out * out, Consume_atoms *)
{
	assert(in->c1);
	//assert(in->c2 == NULL);
	delete in->c1;
	return 0;
}

} // namespace basicplug2
