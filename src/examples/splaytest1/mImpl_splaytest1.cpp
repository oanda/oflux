#include "OFluxGenerate_splaytest1.h"
#include <stdio.h>
#include <unistd.h>

int gCount = 0;

int S(const S_in *, S_out * out, S_atoms *)
{
	oflux::PushTool<S_out> pt(out);
	pt->b = gCount++;
	pt.next();
	pt->b = gCount++;
	pt.next();
	pt->b = gCount++;
	pt.next();
	usleep(1000);
	return 0;
}

int NS(const NS_in *in, NS_out * out, NS_atoms *)
{
	printf("ns %d\n", in->b);
	usleep(1000);
	return 0;
}
