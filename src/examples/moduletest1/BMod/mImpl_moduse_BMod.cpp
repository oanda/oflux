#include "OFluxGenerate_moduse_BMod.h"
#include <stdio.h>

namespace BMod {

int finish(const finish_in *in, finish_out *, finish_atoms *)
{
	printf("fin %d -> %d\n",in->k,in->r);
	return 0;
}

} // namespace
