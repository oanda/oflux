#include <iostream>
#include "OFluxGenerate_kernalex_plugin3.h"

namespace plugin3 {

bool icond2( int i )
{
        return ( i % 7 ) == 0;
}
bool icond3( int i )
{
        return ( i % 13 ) == 1;
}

int n3 (const n3_in *in, n3_out *out, n3_atoms * )
{
        std::cout << "n3 : " << in->id << std::endl;
        out->id = in->id;
        return 0;
}

}
