#include "OFluxGenerate.h"
#include "mImpl_AMod.h"
#include "atomic/OFluxAtomicInit.h"

void init(int , char **)
{
        OFLUX_MODULE_INIT(AMod,aB_aA,"aB_aA inst",1);
        // aB instance not inited
}

int consumer_even(const consumer_even_in *in, consumer_even_out *out, consumer_even_atoms *)
{
	out->k = in->k;
	out->r = in->k * in->k;
	return 0;
}

int consumer_odd(const consumer_odd_in *in, consumer_odd_out *out, consumer_odd_atoms *)
{
	out->k = in->k;
	out->r = in->k * in->k + 1;
	return 0;
}

int consumer_ac(const consumer_ac_in *in, consumer_ac_out *, consumer_ac_atoms *)
{
        return 0;
}
