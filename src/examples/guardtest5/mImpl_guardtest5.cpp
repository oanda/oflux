#include <stdio.h>
#include "OFluxGenerate_guardtest5.h"
#include "atomic/OFluxAtomicInit.h"

int S(const S_in *, S_out * out, S_atoms * atoms)
{
    sleep(1);
    printf("S wakeup\n");

	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
    printf("NS enter\n");

	return 0;
}

void init(int, char * argv[])
{
}
