#include "OFluxGenerate.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int Three(const Three_in *in, Three_out *out, Three_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Three\n";
#endif
    ++nodeCount_Three;
    return 0;
}
