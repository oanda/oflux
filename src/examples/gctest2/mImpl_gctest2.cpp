#include <stdio.h>
#include "OFluxGenerate_gctest2.h"
#include "atomic/OFluxAtomicInit.h"
#include "OFlux.h"

int 
S_splay(const S_splay_in *, S_splay_out * out, S_splay_atoms * atoms)
{
	static int x = 0;

	oflux::PushTool<S_splay_out> ptool(out);
	for(size_t i = 0; i < 10; ++i) {
		ptool->a = (++x)%10;
		ptool.next();
	}
	return 0;
}

int 
NS(const NS_in *in, NS_out *out, NS_atoms * atoms)
{
	int * & ab = atoms->ab();
	static int x = 0;
	out->a = in->a;
	printf("NS %d\n",in->a);
	if(ab == NULL) {
		ab = new int(in->a);
	}
	return 0;
}

int
NSS(const NSS_in * in, NSS_out *, NSS_atoms * atoms)
{
	int * & ab = atoms->ab();
	delete ab;
	ab = NULL;
	return 0;
}

void init(int, char * argv[])
{
        printf("watch for memory growth\n");
}
