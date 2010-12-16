
#include "OFluxGenerate_bigsleep.h"
#include "OFluxRunTime.h"
#include "OFluxProfiling.h"
#include <signal.h>

extern oflux::shared_ptr<oflux::RunTimeAbstract> theRT;

void handlesigusr1(int)
{
    signal(SIGUSR1,handlesigusr1);
    if(theRT.get()) theRT->soft_kill();
}
void handlesigint(int)
{
    signal(SIGUSR2,handlesigint);
    if(theRT.get()) theRT->hard_kill();
}

void init(int, char * argv[])
{
    signal(SIGUSR1,handlesigusr1);
    signal(SIGUSR2,handlesigint);
    printf(" signal handlers installed (USR1 - softkill)"
                " (INT - hardkill)\n");
    fflush(stdout);
}


int Start(const Start_in *, Start_out *, Start_atoms *)
{
        //sleep(10000);
	usleep(10000 * 1000); 
	theRT->log_snapshot();
	return 0;
}
int Start2(const Start2_in *, Start2_out *, Start2_atoms *)
{
        //sleep(10000);
	usleep(10000 *1000); 
        return 0;
}
