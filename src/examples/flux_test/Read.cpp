#include "OFluxGenerate.h"
#include <iostream>
#include <unistd.h>

using namespace std;

int Read(const Read_in *in, Read_out *out, Read_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Read\n";
#endif
    ++nodeCount_Read;
    return 0;
}
