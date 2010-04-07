#include <stdio.h>
#include "OFluxGenerate_multiguard.h"
#include "atomic/OFluxAtomicInit.h"

int S(const S_in *, S_out * out, S_atoms * atoms)
{
	int * & ptr = atoms->g();
	assert(ptr);
	out->b = ++(*ptr) % 100000;
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
	printf("ns out: %d\n", in->b);
	return 0;
}
	
void init(int, char * argv[])
{
	OFLUX_GUARD_POPULATER(Ga,GaPop);
	Ga_key gak;
	GaPop.insert(&gak,new int(0));
	OFLUX_GUARD_POPULATER(Gb,GbPop);
	Gb_key gbk;
	GbPop.insert(&gbk,new int(20000));
}
