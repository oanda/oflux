#include <stdio.h>
#include "OFluxGenerate_guardprecedence.h"
#include "atomic/OFluxAtomicInit.h"


int S(const S_in *, S_out * out, S_atoms * atoms)
{
	int * & ptr = atoms->Ga();
	assert(ptr);
	out->b = ++(*ptr) % 10;
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
	int * & ptr = atoms->Gb();
	printf("ns out: %d is %s %p\n", in->b, (ptr == NULL ? "    null" : "not null"), &ptr);
	return 0;
}

void init(int, char * argv[])
{
	OFLUX_GUARD_POPULATER(Ga,GaPop);
	Ga_key gak;
	GaPop.insert(&gak,new int(0));
	OFLUX_GUARD_POPULATER(Gb,GbPop);
	Gb_key gbk;
	gbk.v = 2;
	GbPop.insert(&gbk,new int(2));
}
