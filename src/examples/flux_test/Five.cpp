#include "OFluxGenerate.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int Five(const Five_in *in, Five_out *out, Five_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Five\n";
#endif
    ++nodeCount_Five;
    return 0;
}
