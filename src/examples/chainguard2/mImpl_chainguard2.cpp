#include "OFluxGenerate_chainguard2.h"
#include "OFluxRunTimeBase.h"
#include <unistd.h>

extern boost::shared_ptr<oflux::RunTimeBase> theRT;

bool
isNotNull(void * x)
{ return x != NULL; }

class Cob {
public:
	Cob(int c) : _c(c) {}

	int
	c() const { return _c; }

	void
	add(int val) { _c += val; }
private:
	int _c;
};

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

	Aob * & ga = atoms->ga();
	Bob * & gb = atoms->gb();

	bool ga_new = false;
	bool gb_new = false;
        bool gc_new = false;

	if(ga == NULL) {
		ga = new Aob(in_a%10);
		ga_new = true;
	}

	if(atoms->have_gb()) {
		if(gb == NULL) {
			gb = new Bob(in_a);
			gb_new = true;
		} else {
			gb->add(in_a);
		}

                b = gb->get_b();

		printf("N %d %d %d %s %s\n"
			, in->a
			, ga->get_a()
			, gb->get_b()
			, tfstr(ga_new)
			, tfstr(gb_new));
	}

        if(atoms->have_gc_()) {
		Cob * & gc = atoms->gc_();

		if(gc == NULL) {
			gc = new Cob(b);
			gc_new = true;
		} else {
			gc->add(b);
		}

		printf("N %d %d %d %d %s %s %s\n"
			, in->a
			, ga->get_a()
			, gb->get_b()
                        , gc->c()
			, tfstr(ga_new)
			, tfstr(gb_new)
                        , tfstr(gc_new));
	}

        // ga always grabbed or created
        if(!atoms->have_gb() && !atoms->have_gc_()) {
		printf("N %d %d %s have no gb have no gc\n"
			, in->a
			, ga->get_a()
			, tfstr(ga_new));
	} else if (atoms->have_gb() && !atoms->have_gc_()){
		printf("N %d %d %d %s %s have no gc\n"
			, in->a
			, ga->get_a()
                        , gb->get_b()
			, tfstr(ga_new)
                        , tfstr(gb_new));
	}

	return 0;
}
