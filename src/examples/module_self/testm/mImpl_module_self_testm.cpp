#include "OFluxGenerate_module_self_testm.h"

namespace testm {

int Process(const Process_in * in, Process_out * out, Process_atoms * atoms)
{
    out->txt = in->txt + "BLAH";

    return 0;
}
}
