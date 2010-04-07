#include "OFluxGenerate.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int Two(const Two_in *in, Two_out *out, Two_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Two\n";
#endif
    ++nodeCount_Two;
    return 0;
}
