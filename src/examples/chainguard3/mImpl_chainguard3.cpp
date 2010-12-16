#include "OFluxGenerate_chainguard3.h"
#include "OFluxRunTimeBase.h"
#include <unistd.h>

extern oflux::shared_ptr<oflux::RunTimeBase> theRT;

bool
isNotNull(void * x)
{ return x != NULL; }

bool
isNull(void * x)
{ return x == NULL; }

#define tfstr(B) (B ? "true" : "false")

int
S(        const S_in *
	, S_out * out
	, S_atoms *)
{
	static int a = 0;
	out->a = (++a)%10;
	if(a > 10000) {
		printf("S soft_kill request\n");
		theRT->soft_kill();
		a = 0;
	}
	usleep(100);
	printf("S -> %d\n", out->a);
	return 0;
}

int
N(        const N_in * in
	, N_out *
	, N_atoms * atoms)
{
	int in_a = in->a;
        int b = 0;

	bool have_ga = atoms->have_ga();
	int * & ga = atoms->ga();
	int old_ga = (ga ? *ga : -1);
	if(have_ga && ga == NULL) {
		ga = new int(in_a);
	}
	int new_ga = *ga;
	bool have_gb = atoms->have_gb();
	int old_gb = -1;
	int new_gb = -1;
	if(have_gb) {
		int * & gb = atoms->gb();
		old_gb = (gb ? *gb : -1);
		new_gb = old_gb;
		if(gb == NULL) {
			gb = new int(in_a * 100);
			new_gb = *gb;
		}
	}
	printf("N: ga[%d] %s %d->%d  gb %s %d->%d\n"
		, in_a
		, (have_ga ? "have" : "    ")
		, old_ga
		, new_ga
		, (have_gb ? "have" : "    ")
		, old_gb
		, new_gb);

	return 0;
}
