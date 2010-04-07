#include "OFluxGenerate_simplemodule_testm.h"

namespace testm {

int Process(const Process_in * in, Process_out * out, Process_atoms * atoms)
{
    out->txt = in->txt + "BLAH";

    return 0;
}
}
