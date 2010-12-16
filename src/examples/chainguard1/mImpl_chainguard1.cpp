#include <stdio.h>
#include "OFluxGenerate_chainguard1.h"
#include "OFluxRunTimeAbstract.h"
#include <unistd.h>

extern oflux::shared_ptr<oflux::RunTimeAbstract> theRT;

bool
isNotNull(Aob * x) 
{ return x != NULL; }

class Bob {
public:
	Bob(int a) : _a(a) {}

	int 
	a() const { return _a; }

	void
	add(int a) { _a += a; }
private:
	int _a;
};

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
	int a = in->a;
	Aob * & ga = atoms->ga();
	bool ga_new = false;
	bool gb_new = false;
	if(ga == NULL) {
		ga = new Aob(a%10);
		ga_new = true;
	}
	if(atoms->have_gb()) {
		Bob * & gb = atoms->gb();
		if(gb == NULL) {
			gb = new Bob(a);
			gb_new = true;
		} else {
			gb->add(a);
		}
#define tfstr(B) (B ? "true" : "false")
		printf("N %d %d %d %s %s\n"
			, in->a
			, ga->id()
			, gb->a()
			, tfstr(ga_new)
			, tfstr(gb_new));
	} else {
		printf("N %d %d   have no gb %s\n"
			, in->a
			, ga->id()
			, tfstr(ga_new));
	}
	return 0;
}
