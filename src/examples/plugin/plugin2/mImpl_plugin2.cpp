#include <iostream>
#include "OFluxGenerate_kernalex_plugin2.h"

extern "C" {

void init__plugin2(void * params)
{
        std::cout << "init__plugin2() called here\n";
}

void deinit__plugin2()
{
	std::cout << "deinit_plugin2() called here\n";
}

} // "C"

namespace plugin2 {

bool IsPlugin2( int i )
{
        return ( i % 2 ) == 0;
}

int ExecutePlugin2 (const ExecutePlugin2_in *, ExecutePlugin2_out *, ExecutePlugin2_atoms * )
{
        std::cout << "ExecutePlugin2" << std::endl;
        return 0;
}

}
