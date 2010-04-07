#include "OFluxGenerate_kernalex_mod2.h"

namespace mod2 {

int pass(const pass_in * in, pass_out * out, pass_atoms * atoms)
{
        out->id = -(in->id);
        return 0;
}

}

