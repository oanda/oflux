#include <stdio.h>
#include "OFluxGenerate_gctest1.h"
#include "atomic/OFluxAtomicInit.h"

int S(const S_in *, S_out * out, S_atoms * atoms)
{
	static int x = 0;
        out->a = ++x;
	return 0;
}

int 
NS(const NS_in *in, NS_out *out, NS_atoms * atoms)
{
	int * & ab = atoms->ab();
	static int x = 0;
	out->a = in->a;
	if(ab == NULL) {
		ab = &x;
	}
	return 0;
}

int
NSS(const NSS_in * in, NSS_out *, NSS_atoms * atoms)
{
	int * & ab = atoms->ab();
	ab = NULL;
	return 0;
}

void init(int, char * argv[])
{
        printf("watch for memory growth\n");
}
