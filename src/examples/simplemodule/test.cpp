#include "OFluxGenerate.h"


int Start(const Start_in * in, Start_out * out, Start_atoms * atoms)
{
    out->txt = "hello";
    return 0;
}

int End(const End_in * in, End_out * out, End_atoms * atoms)
{
    printf("%s\n", in->txt.c_str());
    return 0;
}

