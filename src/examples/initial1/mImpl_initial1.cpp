#include "OFluxGenerate_initial1.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


int Src(const Src_in *, Src_out * , Src_atoms *)
{
        printf("[%lu] Src()\n",(unsigned long)pthread_self());
        sleep(2);
        return 0;
}

int Init(const Init_in *in, Init_out *, Init_atoms *)
{
        printf("[%lu] Init()\n",(unsigned long)pthread_self());
        return 0;
}



