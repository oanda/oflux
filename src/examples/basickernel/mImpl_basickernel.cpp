#include "OFluxGenerate_basickernel.h"
#include "OFluxRunTime.h"
#include "OFluxIOConversion.h"
#include "OFluxLogging.h"
#include <iostream>

static int id = 0;

int 
Foo(const Foo_in *, Foo_out * out, Foo_atoms *)
{
    out->a = id++;
    std::cout << "Foo : " << id << std::endl;
    return 0;
}

int
Toast(const Toast_in *, Toast_out *, Toast_atoms *)
{
	return 0;
}

using namespace oflux;
boost::shared_ptr<RunTimeAbstract> theRT;
