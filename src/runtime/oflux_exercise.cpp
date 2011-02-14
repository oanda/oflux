//
// This program will take a flow XML and run it in dummy mode.
// This means that it will run every node with a dummy function and
// exercise the flow with all of its guard acquisitions.  
// It can be used to test the flow for bottlenecks or other effects.
// It may also indicate interesting effects related to choice of runtime.
// Limitations:
//  * guards are reduced to singleton (no () key guards)
//  * no data is passed since the replacement nodes do not pass inputs or
//     outputs

#include "OFlux.h"
#include "flow/OFluxFlowExerciseFunctions.h"
#include "flow/OFluxFlow.h"
#include "atomic/OFluxAtomic.h"
#include "lockfree/atomic/OFluxLFAtomicMaps.h"
#include "atomic/OFluxAtomicHolder.h"
#include "OFluxRunTimeBase.h"
#include "OFluxIOConversion.h"
#include "OFluxLogging.h"
#include <iostream>
#include <signal.h>


oflux::shared_ptr<oflux::RunTimeAbstract> theRT;

void
init_atomic_maps(int) {}

oflux::flow::exercise::AtomicSetAbstract * atomic_set = NULL;
oflux::flow::Flow * flow = NULL;

void handlesighup(int)
{
	signal(SIGHUP,handlesighup);
	atomic_set->report();
	oflux::flow::exercise::node_report(flow);
	theRT->log_snapshot();
}

oflux::flow::exercise::AtomicAbstract::P * _atomic_array;

int
main(int argc, char * argv[])
{
	if(argc <= 1) {
		std::cout << "provide an XML flow file argument\n";
		return 9;
	}
	char * max_nsec_wait_str = getenv("EXERCISE_SLEEP");
	if(max_nsec_wait_str) {
		oflux::flow::exercise::max_nsec_wait = atol(max_nsec_wait_str);
	}
	int init_threads = 0;
	char * init_threads_str = getenv("EXERCISE_THREADS");
	if(init_threads_str) {
		init_threads = atoi(init_threads_str);
	}
	oflux::RunTimeConfiguration rtc = {
		  1024*1024 // stack size
		, init_threads // initial threads (ignored really)
		, 0 // max threads
		, 0 // max detached
		, 0 // thread collection threshold
		, 1000 // thread collection sample period (every N node execs)
		, argv[1] // XML file
		, NULL // temporary
		, "xml" // xml subdir for plugins
		, "lib" // lib subdir for plugins
		, NULL
		, init_atomic_maps
		};
	oflux::logging::toStream(std::cout); 
	oflux_log_info(" exercise::max_nsec_wait is %ld\n", oflux::flow::exercise::max_nsec_wait);
	oflux::EnvironmentVar env(oflux::runtime::Factory::classic);
	oflux::flow::ExerciseFunctionMaps ffmaps(env.runtime_number == 4);
	atomic_set = ffmaps.atomic_set();
	rtc.flow_maps = &ffmaps;
	theRT.reset(oflux::runtime::Factory::create(env.runtime_number, rtc));
	flow = theRT->flow();
	oflux::flow::exercise::AtomicAbstract::P atomic_array[atomic_set->size()+1];
	_atomic_array = &atomic_array[0];
	atomic_set->fill(atomic_array);
	signal(SIGHUP,handlesighup);
	if(!env.nostart) {
		theRT->start();
	}
	theRT.reset(); // force deletion of the runtime
	
	return 0;
}
