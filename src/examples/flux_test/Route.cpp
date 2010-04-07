#include "OFluxGenerate.h"
#include <iostream>

using namespace std;

int Route(const Route_in *in, Route_out *out, Route_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Route\n";
#endif
    ++nodeCount_Route;
    return 0;
}
