#include <stdio.h>
#include "OFluxGenerate_poolguard.h"
#include "atomic/OFluxAtomicInit.h"

int c = 0;

int S(const S_in *, S_out * out, S_atoms * atoms)
{
	int * & ptr = atoms->I();
	assert(ptr);
	out->b = (*ptr) + c * 3 ;
        c++;
	printf("s out: %d is %s off pooled int %d\n", out->b, (ptr == NULL ? "    null" : "not null"), (unsigned int)*ptr);
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
	int * & ptr = atoms->I();
	printf("ns out: %d is %s off pooled int %d\n", in->b, (ptr == NULL ? "    null" : "not null"), (unsigned int)*ptr);
	return 0;
}
	
void init(int, char * argv[])
{
	OFLUX_GUARD_POPULATER(IntPool,IntPoolPop);
        IntPool_key ipk;
	IntPoolPop.insert(&ipk,new int(0));
	IntPoolPop.insert(&ipk,new int(1));
	IntPoolPop.insert(&ipk,new int(2));
}
