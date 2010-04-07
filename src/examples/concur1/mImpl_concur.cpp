#include "OFluxGenerate_concur.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

bool isEven(int a)
{
        return (a%2) == 0;
}

int Src(const Src_in *, Src_out * out, Src_atoms *)
{
        static int actr = 0;
        out->a = ++actr;
        usleep(1000);
        return 0;
}

int Exec1(const Exec1_in *in, Exec1_out *, Exec1_atoms *)
{
        printf("[%lu] Exec1(%d)\n",(unsigned long)pthread_self(), in->a);
        return 0;
}
int Exec2(const Exec2_in *in, Exec2_out *, Exec2_atoms *)
{
        printf("[%lu] Exec2(%d)\n",(unsigned long)pthread_self(), in->a);
        return 0;
}
int Exec3(const Exec3_in *in, Exec3_out *, Exec3_atoms *)
{
        printf("[%lu] Exec3(%d)\n",(unsigned long)pthread_self(), in->a);
        return 0;
}
int Exec4(const Exec4_in *in, Exec4_out *, Exec4_atoms *)
{
        printf("[%lu] Exec4(%d)\n",(unsigned long)pthread_self(), in->a);
        return 0;
}



