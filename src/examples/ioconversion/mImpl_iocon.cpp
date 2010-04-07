#include "OFluxGenerate_iocon.h"
#include <stdio.h>
#include <unistd.h>

int A(const A_in *, A_out * out, A_atoms *)
{
        static int a = 0;
        out->a = ++a;
        out->b = 'a' + (char)(a%26);
        out->c = a * ((long)a);
        usleep(1000 * 1000);
        printf("A : : %d %c %ld\n", out->a, out->b, out->c);
        return 0;
}

int B(const B_in *in, B_out * out, B_atoms *)
{
        out->d = in->a;
        out->e = 'z' - (char)(in->a % 26);
        printf("B : %d : %d %c\n", in->a, out->d, out->e);
        return 0;
}

int C(const C_in *in, C_out *, C_atoms *)
{
        printf("C : : %c\n",in->e);
        return 0;
}

int D(const D_in *in, D_out *, D_atoms *)
{
        printf("D : : %ld\n",in->c);
        return 0;
}
