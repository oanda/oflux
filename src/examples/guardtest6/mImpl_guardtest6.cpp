#include <stdio.h>
#include "OFluxGenerate_guardtest6.h"
#include "OFluxRunTime.h"
#include "OFluxLogging.h"
#include <iostream>
#include <unistd.h>
#include <stdlib.h>

extern oflux::runtime::classic::RunTime *theRT;

int atomicityCheck = 0;

int S1(const S1_in *, S1_out * out, S1_atoms * atoms)
{
    // Sleep from 5ms to 7ms
    int sleepUs = rand() % 5000 + 2000;
	usleep(sleepUs);
	return 0;
}

int NS1(const NS1_in *in, NS1_out *, NS1_atoms * atoms)
{
    assert(!atomicityCheck);
    atomicityCheck = -11;

    // Sleep from 100us to 700us
    int sleepUs = rand() % 600 + 100;
    usleep(sleepUs);

    atomicityCheck = 0;

	return 0;
}

int S2(const S2_in *, S2_out *out, S2_atoms * atoms)
{
    sleep(1);

    oflux::PushTool<S2_out> pt(out);

    for (int i = 0; i < 300; ++i) {
        pt->k = 0; 
        pt.next();
    }

	return 0;
}

int NS2(const NS2_in *in, NS2_out *, NS2_atoms * atoms)
{
    assert(atomicityCheck >= 0);
    atomicityCheck = 1;

    // Sleep from 100us to 700us
    int sleepUs = rand() % 600 + 100;
    usleep(sleepUs);
    
    atomicityCheck = 0;

	return 0;
}
