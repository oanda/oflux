#include <stdio.h>
#include "OFluxGenerate_guardtest4.h"
#include "OFluxRunTime.h"
#include "OFluxLogging.h"
#include <iostream>
#include <unistd.h>

extern oflux::runtime::classic::RunTime *theRT;

int S(const S_in *, S_out * out, S_atoms * atoms)
{
    printf("S going to sleep\n");
	sleep(5);
	return 0;
}

int NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
    printf("NS - ++++++++++++++++\n");
    usleep(10000);
	return 0;
}

int R(const R_in *, R_out *, R_atoms * atoms)
{
	usleep(100000);
	return 0;
}

int NR(const NR_in *in, NR_out *, NR_atoms * atoms)
{
    printf("NR - ******\n");
    usleep(10000);
	return 0;
}

int Log(const Log_in *in, Log_out *, Log_atoms * atoms)
{
    oflux::logging::toStream(std::cout);
    sleep(20);
    theRT->log_snapshot();
    theRT->log_snapshot_guard("Ga");
    theRT->log_snapshot_guard("Gb");
    return 0;
}
