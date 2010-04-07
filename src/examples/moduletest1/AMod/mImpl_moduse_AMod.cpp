#include <stdio.h>
#include <unistd.h>
#include "OFluxGenerate_moduse_AMod.h"
#include <cassert>
#include <stdlib.h>
#include <string.h>

namespace AMod {

ModuleConfig::ModuleConfig(const char * n, int i)
        : id(i)
{
        strncpy(name,n,39); // not best practices here -- just for an example
}

ModuleConfig * init(const char * n, int id) 
{
        ModuleConfig * mc = NULL;
        assert(mc == NULL);
        mc = new ModuleConfig(n,id);
        return mc;
}

bool even(int a)
{
	return (a%2) == 0;
}

int src(const src_in *, src_out * out, src_atoms * atoms)
{
	static int i = 0;
	out->a = ++i;
	usleep(10000);
        if(out->a % 1000 == 0) {
                printf("AMod::src output for instance %s %d\n", atoms->self()->name,
                        atoms->self()->id);
        }
	return 0;
}

} // namespace
