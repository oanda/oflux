#include <stdio.h>
#include "OFluxGenerate_guardtest2.h"
#include <unistd.h>

int S(const S_in *, S_out * out, S_atoms * atoms)
{
	int * & ptr = atoms->Ga();
	if(ptr == NULL) {
		ptr = new int(0);
	}
	out->b = ++(*ptr) % 10;
	usleep(1000);
	printf("s out\n");
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
	const int * ptr = atoms->Gb();
	printf("ns out: %d is %s %p\n", in->b, (ptr == NULL ? "    null" : "not null"), ptr);
	return 0;
}
	
int R(const R_in *, R_out *, R_atoms * atoms)
{
	const int * ptr __attribute__((unused)) = atoms->Ga();
	usleep(1000);
	printf("r out\n");
	return 0;
}
