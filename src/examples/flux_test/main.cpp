#include "OFluxRunTime.h"
#include "OFluxGenerate.h"
#include "OFluxLogging.h"
#include <iostream>

using namespace oflux;
RunTime * theRT = NULL;

namespace ofluximpl {
 void init_atomic_maps(int style);
} // namespace ofluximpl

int main(int argc, char * argv[])
{
	init(argc-1,&(argv[1]));
	
	assert(argc >= 2);

        FlowFunctionMaps ffmaps(
                  ofluximpl::__conditional_map
                , ofluximpl::__master_create_map
                , ofluximpl::__theGuardTransMap
                , ofluximpl::__atomic_map_map
                , ofluximpl::__ioconverter_map);
        RunTimeConfiguration rtc = {
                  1024*1024 // stack size
                , 1 // initial threads (ignored really)
                , 0 // max threads
                , 0 // max detached
                , 0 // thread collection threshold
                , 1000 // thread collection sample period (every N node execs)
                , argv[1] // XML file
                , &ffmaps
		, ofluximpl::init_atomic_maps
        };

	//rtc.prioritize_queued_sources = false;
	RunTime rt(rtc);
	theRT = &rt;
	//loggingToStream(std::cout); // comment out if no oflux logging is desired

	rt.start();
	
	return 0;
}

