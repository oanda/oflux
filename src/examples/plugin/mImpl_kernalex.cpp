#include "OFluxGenerate_kernalex.h"
#include "OFluxRunTime.h"
#include "OFluxIOConversion.h"
#include "OFluxLogging.h"
#include <iostream>

static int id = 1;

int zRequestSrc(const zRequestSrc_in *, zRequestSrc_out * out, zRequestSrc_atoms * atoms)
{
    std::cout << "RequestSrc : " << id << std::endl;
    out->id = id++;
	return 0;
}

 
int zExecute(const zExecute_in *in, zExecute_out * out, zExecute_atoms * atoms)
{
    std::cout << "Execute : " << in->id << std::endl;
    out->id = in->id;
	return 0;
}

int Sink(const Sink_in *in, Sink_out * out, Sink_atoms * atoms)
{
    std::cout << "Sink : " << in->id << std::endl;
	return 0;
}

using namespace oflux;
boost::shared_ptr<RunTimeAbstract> theRT;
