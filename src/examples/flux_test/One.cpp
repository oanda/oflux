#include "OFluxGenerate.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int One(const One_in *in, One_out *out, One_atoms *)
{
#ifdef DO_OUTPUT
    cout << "One\n";
#endif
    ++nodeCount_One;
    return 0;
}
