#include <stdio.h>
#include "OFluxGenerate_rwguard.h"
#include "atomic/OFluxAtomicInit.h"
#include "OFlux.h"
#include "OFluxThreads.h"
#include <pthread.h>

int S(const S_in *, S_out * out, S_atoms * atoms)
{
	int * & ptr = atoms->G();
	assert(ptr);
	oflux::PushTool<S_out> ptool(out);
	for(int j = 0 ; j < 10; ++j) {
		ptool->b = (++(*ptr) % 10) * 10 + j;
		ptool.next();
	}
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
	const int * ptr = atoms->G();
	printf("::" PTHREAD_PRINTF_FORMAT " ns out: %d is %d %p\n"
		, pthread_self()
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
