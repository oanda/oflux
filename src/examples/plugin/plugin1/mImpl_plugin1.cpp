#include <iostream>
#include "OFluxGenerate_kernalex_plugin1.h"
#include "atomic/OFluxAtomicInit.h"

extern "C" {

void init__plugin1(void * params)
{
        int * v = reinterpret_cast<int*>(params);
        std::cout << "init__plugin1() called here - params: " << *v << std::endl;

        {
                using namespace plugin1;
                OFLUX_GUARD_POPULATER_PLUGIN(plugin1,ig1,pop1);
                int k = 1;
                pop1.insert(&k,new int(3));
        }
}

void deinit__plugin1()
{
	std::cout << "deinit_plugin1() called here\n";
}

} // "C"

namespace plugin1 {

bool pcond1( int i)
{
        return ((i%3) == 0);
}

bool icond1( int i)
{
        return ((i%5) == 0);
}

int exec1(const exec1_in * in, exec1_out * out, exec1_atoms * atoms)
{
        std::cout << "exec1 : " << in->id << std::endl;
        out->aid = in->id;
        return 0;
}

int n1(const n1_in * in, n1_out * out, n1_atoms * atoms)
{
        std::cout << "n1 : " << in->id << std::endl;
        return 0;
}

int n2(const n2_in * in, n2_out * out, n2_atoms * atoms)
{
        std::cout << "n2 : " << in->id << std::endl;
        return 0;
}

}
