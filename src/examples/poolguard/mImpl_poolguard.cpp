#include <stdio.h>
#include "OFluxGenerate_poolguard.h"
#include "atomic/OFluxAtomicInit.h"
#include "OFluxRunTimeAbstract.h"
#include "OFluxThreads.h"
#include <pthread.h>

int c = 0;

extern oflux::shared_ptr<oflux::RunTimeAbstract> theRT;

int S(const S_in *, S_out * out, S_atoms * atoms)
{
	int * & ptr = atoms->I();
	assert(ptr);
	out->b = (*ptr) + c * 3 ;
        c++;
	printf(":::[" PTHREAD_PRINTF_FORMAT "] s out: %d is %s off pooled int %d\n"
		, pthread_self()
		, out->b
		, (ptr == NULL ? "    null" : "not null")
		, (unsigned int)*ptr);
	//if(c > 100) theRT->soft_kill();
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
	int * & ptr = atoms->I();
	printf(":::[" PTHREAD_PRINTF_FORMAT "] ns out: %d is %s off pooled int %d\n"
		, pthread_self()
		, in->b, (ptr == NULL ? "    null" : "not null")
		, (unsigned int)*ptr);
	return 0;
}
	
void init(int, char * argv[])
{
	OFLUX_GUARD_POPULATER(IntPool,IntPoolPop);
        IntPool_key ipk;
	int * ip = NULL;
	for(size_t i = 0 ; i < 3; ++i) {
		ip = new int(i);
		IntPoolPop.insert(&ipk,ip);
		printf("pool element at %p is %d\n", ip, *ip);
	}
}
