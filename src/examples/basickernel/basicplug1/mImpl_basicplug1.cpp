#include "OFluxGenerate_basickernel_basicplug1.h"
#include <cassert>

namespace basicplug1 {

int
Bar(const Bar_in * in, Bar_out * out, Bar_atoms *)
{
	B1 b1;
	out->op1 = b1;
	out->op2 = new int(in->a);
	out->dc = false;
	return 0;
}

int
Consume(const Consume_in * in, Consume_out * out, Consume_atoms *)
{
	assert(in->op2);
	delete in->op2;
	return 0;
}

} // namespace basicplug1
