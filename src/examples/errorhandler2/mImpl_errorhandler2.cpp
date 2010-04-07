#include "OFluxGenerate_errorhandler2.h"
#include "OFluxRunTimeAbstract.h"
#include "OFlux.h"
#include <iostream>
#include <unistd.h>

int count = 0;

extern boost::shared_ptr<oflux::RunTimeAbstract> theRT;

int B(const B_in * in, B_out * out, B_atoms *)
{
	oflux::PushTool<B_out> ptool(out);
	ptool.next();
	ptool.next();
	ptool.next();
	::usleep(1);
	return 0;
}

int A(const A_in * in, A_out * out, A_atoms *)
{
	return 0;
}

int Node1(const Node1_in * in, Node1_out * out, Node1_atoms * atoms)
{
    if(++count > 1000) {
	theRT->soft_kill();
    }
    out->arg.set(2);
    out->foo = count;
    out->bar = 1-count;
    ::usleep(10);
    return 0;
}

int Node2(const Node2_in * in, Node2_out * out, Node2_atoms * atoms)
{
    std::cout << "Node2 - arg = " << in->arg.get() 
	<< " foo " << in->foo << std::endl;
    return -1;
}

int Error(const Error_in * in, Error_out * out, Error_atoms * atoms, int err)
{
    std::cout << "Error - arg = " << in->arg.get() 
	<< " foo = " << in->foo
	<< " err = " << err << std::endl;
    return 0;
}


void init(int, char * argv[])
{
	
}
