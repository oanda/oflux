#include "OFluxGenerate_splayordering.h"
#include "OFluxRunTimeAbstract.h"
#include <unistd.h>
#include <iostream>
#include "atomic/OFluxAtomicInit.h"

extern boost::shared_ptr<oflux::RunTimeAbstract> theRT;

void init(int, char * argv[])
{
	OFLUX_GUARD_POPULATER(First,FirstPop);
	First_key fk;
	FirstPop.insert(&fk,new size_t(0));
	FirstPop.insert(&fk,new size_t(0));
	FirstPop.insert(&fk,new size_t(0));
}


int 
NumberGenerator_splay(
	  const NumberGenerator_splay_in * in
	, NumberGenerator_splay_out * out
	, NumberGenerator_splay_atoms * atoms)
{
    static size_t num = 1;
    static size_t count = 0;
    oflux::PushTool<NumberGenerator_splay_out> ptool(out);

    //std::cout << __FUNCTION__ << std::endl;

    if(++count % 100 == 0) {
        ::usleep(100);
    }
    for(size_t i = 0 ; i < 10 ; ++i) {
	ptool->val = num++;
	ptool.next();
    }
    if(count > 15000) {
        theRT->soft_kill();
    }
    if(count > 30000) {
        theRT->hard_kill();
    }
    return 0;
}

int 
Check(
	  const Check_in * in
	, Check_out * out
	, Check_atoms * atoms)
{
    size_t *& last = atoms->Last();
    //std::cout << __FUNCTION__ << std::endl;
    if(!last) {
        last = new size_t(0);
    }
    if(in->val != (*last) + 1) {
        std::cout << "last: " << *last << ", current: " << in->val << std::endl;
        //exit(111);
    }
    //std::cout << in->val << std::endl;
    *last = in->val;
    return 0;
}

int 
Delay(const Delay_in * in, Delay_out * out, Delay_atoms * atoms)
{
    ::usleep(200);
    return 0;
}

int 
AlsoGuard(const AlsoGuard_in * in, AlsoGuard_out * out, AlsoGuard_atoms * atoms)
{
    return 0;
}
