#include "OFluxGenerate_module_self_testm_self.h"

namespace testm_self {

int Process(const Process_in * in, Process_out * out, Process_atoms * atoms)
{
    out->txt = in->txt + "BLAH";

    return 0;
}
}
