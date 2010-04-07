#include <unistd.h>
#include "OFluxGenerate.h"
#include "OFluxRunTime.h"

extern oflux::RunTime * theRT;

int Src(const Src_in *, Src_out * out, Src_atoms *)
{
	static useconds_t slps = 1 * 1000; 
	static int count = 0;
	if(count < 3000) {
		count++;
		out->slp = slps * 200; // will cause a leak of threads
	} else {
		out->slp = slps / 3;
	}
	usleep(slps);
	return 0;
}

int Dst(const Dst_in * in, Dst_out *, Dst_atoms *)
{
	static int count = 0;
	usleep(in->slp);
	count++;
	if(count > 100) {
		theRT->log_snapshot();
		count = 0;
	}
	return 0;
}
