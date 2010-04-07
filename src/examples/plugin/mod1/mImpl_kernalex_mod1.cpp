#include "OFluxGenerate_kernalex_mod1.h"

namespace mod1 {

int pass(const pass_in * in, pass_out * out, pass_atoms * atoms)
{
        out->id = in->id;
        return 0;
}

}

