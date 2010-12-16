#include <stdio.h>
#include "OFluxGenerate_excluguard.h"
#include "atomic/OFluxAtomicInit.h"
#include "OFluxThreads.h"
#include <pthread.h>

int S(const S_in *, S_out * out, S_atoms * atoms)
{
	int * & ptr = atoms->G();
	assert(ptr);
	out->b = ++(*ptr) % 10;
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
	int * & ptr = atoms->G();
	printf("::" PTHREAD_PRINTF_FORMAT " ns out: %d is %d %p\n"
		, oflux_self()
		, in->b
		, (ptr == NULL ? 0 : *ptr)
		, &ptr);
	return 0;
}
	
void init(int, char * argv[])
{
	OFLUX_GUARD_POPULATER(G,GPop);
	G_key gk;
	GPop.insert(&gk,new int(0));
}
