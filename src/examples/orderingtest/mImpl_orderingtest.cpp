#include "OFluxGenerate_orderingtest.h"
#include "OFluxRunTimeAbstract.h"
#include <unistd.h>
#include <iostream>

extern boost::shared_ptr<oflux::RunTimeAbstract> theRT;

int NumberGenerator(const NumberGenerator_in * in, NumberGenerator_out * out, NumberGenerator_atoms * atoms)
{
    static size_t num = 1;
    static size_t count = 0;

    out->val = num++;
    if(++count % 100 == 0) {
        ::usleep(100);
    }
    if(count > 1500) {
        theRT->soft_kill();
    }
    if(count > 3000) {
        theRT->hard_kill();
    }
    return 0;
}

int Check(const Check_in * in, Check_out * out, Check_atoms * atoms)
{
    size_t *& last = atoms->Last();
    if(!last) {
        last = new size_t(0);
    }
    if(in->val != (*last) + 1) {
        std::cout << "last: " << *last << ", current: " << in->val << std::endl;
        exit(111);
    }
    //std::cout << in->val << std::endl;
    *last = in->val;
    return 0;
}

int Delay(const Delay_in * in, Delay_out * out, Delay_atoms * atoms)
{
    ::usleep(10);
    return 0;
}

int AlsoGuard(const AlsoGuard_in * in, AlsoGuard_out * out, AlsoGuard_atoms * atoms)
{
    return 0;
}
