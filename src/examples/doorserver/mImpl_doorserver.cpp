#include "OFluxGenerate_doorserver.h"
#include <cstdio>

int 
S(const S_in *, S_out * out, S_atoms *)
{
	return 0;
}

int 
N(const N_in * in, N_out * out, N_atoms *)
{
	printf("N %d\n", in->a);
	return 0;
}

