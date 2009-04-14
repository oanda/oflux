#ifndef _OFLUX_RUNTIME_BASE_H
#define _OFLUX_RUNTIME_BASE_H

#include "OFlux.h"
#include "OFluxRunTimeAbstract.h"
#include <dlfcn.h>

namespace oflux {

namespace flow {
class FunctionMaps;
class Flow;
class Node;
} // namespace flow

/**
 * @class RunTimeConfiguration
 * @brief struct for holding the run-time configuration of the run time.
 * size of the stack, thread pool size (ignored), flow XML file name and
 * the flow maps structure.
 */
struct RunTimeConfiguration {
	int stack_size;
	int initial_thread_pool_size;
	int max_thread_pool_size;
	int max_detached_threads;
	int min_waiting_thread_collect; // waiting in pool collector
	int thread_collection_sample_period;
	const char * flow_filename;
	flow::FunctionMaps * flow_maps;
	const char * plugin_xml_dir;    // plugin xml directory
	const char * plugin_lib_dir;    // plugin lib directory
	void * init_plugin_params;
};

typedef void (*initShimFnType) (RunTimeAbstract *);

class RunTimeBase : public RunTimeAbstract {
public:
        static initShimFnType initShim;
        
	RunTimeBase(const RunTimeConfiguration & rtc)
                : _rtc(rtc)
                , _running(false)
                , _load_flow_next(false)
                {}
        virtual ~RunTimeBase() {}
        virtual void start() = 0;
        /**
         * @brief schedule a new flow load at some point convenient
         */
        void soft_load_flow() { _load_flow_next = true; }
        /**
         * @brief ask the runtime to shut down -- start() function returns
         */
        void soft_kill() { _running = false; }
        virtual bool running() { return _running; }
        virtual void hard_kill() { soft_kill(); }
        virtual void log_snapshot() = 0;
protected:
	const RunTimeConfiguration   _rtc;
        bool                         _running;
        bool                         _load_flow_next;
};

// list of runtime factory functions:
extern RunTimeBase * _create_classic_runtime(const RunTimeConfiguration &);
extern RunTimeBase * _create_melding_runtime(const RunTimeConfiguration &);

inline RunTimeBase * create_classic_runtime(const RunTimeConfiguration & rtc)
{
        RunTimeBase::initShim = (initShimFnType)dlsym (RTLD_NEXT, "initShim");
        return _create_classic_runtime(rtc);
}
inline RunTimeBase * create_melding_runtime(const RunTimeConfiguration & rtc)
{
        RunTimeBase::initShim = (initShimFnType)dlsym (RTLD_NEXT, "initShim");
        return _create_melding_runtime(rtc);
}

} // namespace oflux

#endif // _OFLUX_RUNTIME_BASE_H
