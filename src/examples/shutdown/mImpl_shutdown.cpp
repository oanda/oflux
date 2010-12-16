#include "OFluxGenerate_shutdown.h"
#include "atomic/OFluxAtomicInit.h"
#include "OFluxLogging.h"
#include "OFluxRunTimeAbstract.h"

extern oflux::shared_ptr<oflux::RunTimeAbstract> theRT;

int init(int argc, char * argv[])
{
        OFLUX_GUARD_POPULATER(G,Gpop);
        int v = 0;
        Gpop.insert(&v,new int(500));
        v++;
        Gpop.insert(&v,new int(501));
        return 0;
}

void deinit()
{
        OFLUX_GUARD_WALKER(G,Gwalk);
        void ** v;
        const void * k;
        while(OFLUX_GUARD_WALKER_NEXT(Gwalk,k,v)) {
                if(*v) {
                        oflux_log_info("deleting %d -> %d\n"
                                , * static_cast<const int *>(k)
                                , ** reinterpret_cast<int **>(v));
                        delete (reinterpret_cast<int *>(*v));
                } else {
                        oflux_log_info("deleting %d -> NULL\n"
                                , * static_cast<const int *>(k));
                }
        }
}

int S(const S_in *in, S_out *out, S_atoms * atoms)
{
        static int c = 0;
        out->n = ++c;
        if(c > 3000) {
                theRT->soft_kill();
        }
        return 0;
}

int N(const N_in *in, N_out *out, N_atoms * atoms)
{
        int * & lg = atoms->g();
        if(lg == NULL) lg = new int(in->n % 10);
        return 0;
}
