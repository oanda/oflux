#include "OFluxGenerate_errorhandler.h"
#include <iostream>
#include <unistd.h>

int Node1(const Node1_in * in, Node1_out * out, Node1_atoms * atoms)
{
    out->arg = 2;
    ::sleep(2);
    return 0;
}

int Node2(const Node2_in * in, Node2_out * out, Node2_atoms * atoms)
{
    std::cout << "Node2 - arg = " << in->arg << std::endl;
    return -1;
}

int Error1(const Error1_in * in, Error1_out * out, Error1_atoms * atoms, int err)
{
    std::cout << "Error1 - arg = " << in->arg << " err = " << err << std::endl;
    out->arg = in->arg;
    return 0;
}

int Error2(const Error2_in * in, Error2_out * out, Error2_atoms * atoms)
{
    std::cout << "Error2 - arg = " << in->arg << std::endl << std::endl;
    return 0;
}

void init(int, char * argv[])
{
	
}
