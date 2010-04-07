#include "OFluxGenerate.h"
#include <iostream>

using namespace std;

int Select(Select_in const* in, Select_out* out, Select_atoms*)
{
#ifdef DO_OUTPUT
    cout << "Select\n";
#endif
    ++nodeCount_Select;
    return 0;
}
