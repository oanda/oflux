#include <stdio.h>
#include "OFluxGenerate_guardtest3.h"
#include "atomic/OFluxAtomicInit.h"

int S(const S_in *, S_out * out, S_atoms * atoms)
{
	//int * & ptr = atoms->Ga();
	//assert(ptr);
	out->b = 2;
        out->a = 19;
        sleep(1);
        printf("S wakeup\n");
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
    printf("NS enter\n");

	printf("a: %d, ab: %d, b: %d, bb: %d\n",
           in->a, in->b, *(atoms->ab()), *(atoms->bb()));

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
	gbk.v = 19;
	GbPop.insert(&gbk,new int(19));
}
