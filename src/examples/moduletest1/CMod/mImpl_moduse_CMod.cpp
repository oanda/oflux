#include "OFluxGenerate_moduse_CMod.h"

namespace CMod {

int anode(const anode_in *, anode_out * out, anode_atoms * atoms)
{
        void * & uhm __attribute__((unused)) = atoms->ang();
        out->a = 99;
        return 0;
}

}
