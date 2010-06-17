#include <cstdio>
#include <ctime>
#include "OFluxGenerate_upgradeable.h"
#include "atomic/OFluxAtomicInit.h"

int 
S(const S_in *, S_out *, S_atoms * atoms)
{
	static int x = 10;
	int * & ptr = atoms->Ga();
	if(!ptr) { 
		printf("%ld S:have it as write\n",time(NULL));
		if(x > 0) {
			--x;
		} else {
			ptr = &x;
		}
	} else {
		printf("%ld S:have it as read\n",time(NULL));
	}
	
	// ok now we have a non-null ptr
	// and if it was not null, its really a read mode guard
	sleep(1);
	return 0;
}

int 
R(const R_in *, R_out *, R_atoms * atoms)
{
	printf("%ld R: has it for read\n", time(NULL));
	sleep(1);
	return 0;
}
