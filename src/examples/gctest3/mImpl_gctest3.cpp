#include <stdio.h>
#include <unistd.h>
#include "OFluxGenerate_gctest3.h"
#include "atomic/OFluxAtomicInit.h"

int
SInsert(const SInsert_in *, SInsert_out *out, SInsert_atoms *)
{
	static size_t num_release = 1;
	static size_t nr0  = 0;
	oflux::PushTool<SInsert_out> pt(out);
	for(size_t i = 0; i < num_release; ++i) {
		pt->a = i + num_release;
		pt.next();
	}
	if(nr0 > num_release) {
		++num_release;
		nr0 = 0;
	} else {
		++nr0;
	}
	usleep(6000);
	return 0;
}

int
DoInsert(const DoInsert_in *in, DoInsert_out *, DoInsert_atoms * atoms)
{
	static int foo = 0;
	int * & g = atoms->g();
	g = &foo;
	//printf(" adding  %d\n",in->a);
	return 0;
}

int
SRemove(const SRemove_in *, SRemove_out *out, SRemove_atoms *)
{
	static size_t i = 0;
	out->b = (i++) % 5;
	//printf("sremove %d\n", out->b);
	usleep(6000);
	return 0;
}

int
FindRemove(const FindRemove_in * in, FindRemove_out *out, FindRemove_atoms *)
{
	OFLUX_GUARD_WALKER(g, Gwalk);
	void ** v;
	const void * k;
	oflux::PushTool<FindRemove_out> pt(out);
	size_t out_count = 0;
	while(OFLUX_GUARD_WALKER_NEXT(Gwalk,k,v)) {
		const int *kint = reinterpret_cast<const int *>(k);
		int *vint = reinterpret_cast<int *>(*v);
		if(vint && ((*kint) % 5 == in->b)) {
			pt->a = *kint;
			pt.next();
			++out_count;
		}
	}
	//printf("fremove %d %u\n",in->b,out_count);
	return out_count ? 0 : -1;
}

int
DoRemove(const DoRemove_in *in, DoRemove_out *, DoRemove_atoms * atoms)
{
	int * & g = atoms->g();
	if(g) {
		*g = 0; // remove
		//printf("removing %d\n",in->a);
	}
	return 0;
}
