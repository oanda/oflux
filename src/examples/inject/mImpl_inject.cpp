
#include "OFluxGenerate_inject.h"
#include "OFluxRunTime.h"
#include "event/OFluxEventInjected.h"
#include <cstdio>

extern oflux::shared_ptr<oflux::RunTimeAbstract> theRT;

int 
Start(const Start_in *, Start_out *, Start_atoms *)
{
	sleep(1); 
	static int counter = 0;
	printf("Start\n");
	OFLUX_DECLARE_INJECTION_VECTOR(evs);
	Internal_in input;
	for(size_t i = 0; i < 3; ++i) {
		input.a = ++counter;
		OFLUX_INSERT_INJECTION_INPUT(evs,Internal,input);
	}
	OFLUX_SUBMIT_INJECTION_VECTOR(evs);
	return 0;
}

int 
Internal(const Internal_in *in, Internal_out *, Internal_atoms *)
{
	printf("internal %d\n",in->a);
	return 0;
}
