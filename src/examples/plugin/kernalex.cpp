#include "OFluxGenerate_kernalex.h"
#include "flow/OFluxFlow.h"
#include "atomic/OFluxAtomic.h"
#include "atomic/OFluxAtomicHolder.h"
#include "OFluxRunTime.h"
#include "OFluxIOConversion.h"
#include "OFluxLogging.h"
#include <signal.h>
#include <iostream>
#include <cstring>

using namespace oflux;
extern boost::shared_ptr<RunTimeAbstract> theRT;

extern int pmain(int argc, char * argv[]);

namespace ofluximpl {
 void init_atomic_maps(int style);
} // namespace ofluximpl

int pmain(int argc, char * argv[])
{
    assert(argc >= 2);
    int initPluginParams = 237;
    static flow::FunctionMaps ffmaps(ofluximpl::__conditional_map, ofluximpl::__master_create_map, ofluximpl::__master_create_door_map, ofluximpl::__theGuardTransMap, ofluximpl::__atomic_map_map, ofluximpl::__ioconverter_map);
    RunTimeConfiguration rtc = {
          1024*1024 // stack size
        , 1 // initial threads (ignored really)
        , 0 // max threads
        , 0 // max detached
        , 0 // thread collection threshold
        , 1000 // thread collection sample period (every N node execs)
        , argv[1] // XML file
        , &ffmaps
        , "xml"
        , "lib"
        , &initPluginParams
	, ofluximpl::init_atomic_maps
    };
    logging::toStream(std::cout); // comment out if no oflux logging is desired
    EnvironmentVar env(runtime::Factory::classic);
    theRT.reset(runtime::Factory::create(env.runtime_number, rtc));
    #ifdef HASINIT
    init(argc-1,&(argv[1]));
    #endif //HASINIT
    return 0;
}

void handlesighup(int)
{
    signal(SIGHUP,handlesighup);
    if(theRT) theRT->soft_load_flow();
}

void handlesigusr1(int)
{
    signal(SIGUSR1,handlesigusr1);
    if(theRT) theRT->soft_kill();
}

int main(int argc, char * argv[])
{
    int res = pmain(argc,argv);
    signal(SIGHUP,handlesighup); // install SIGHUP handling
    signal(SIGUSR1,handlesigusr1); // install SIGHUP handling
    const char * oflux_config=getenv("OFLUX_CONFIG");
    if(oflux_config == NULL || NULL==strstr(oflux_config,"nostart")) {
	theRT->start();
    }
    theRT.reset();
    return res;
}
