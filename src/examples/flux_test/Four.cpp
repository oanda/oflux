#include "OFluxGenerate.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int Four(const Four_in *in, Four_out *out, Four_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Four\n";
#endif
    ++nodeCount_Four;
    return 0;
}
