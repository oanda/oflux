#ifndef _OFLUX_RUNTIME_BASE_H
#define _OFLUX_RUNTIME_BASE_H

#include "OFlux.h"
#include "OFluxRunTimeAbstractForShim.h"
#include <dlfcn.h>
#include <vector>
#include <string>

namespace oflux {

namespace flow {
class FunctionMaps;
class Flow;
class Node;
} // namespace flow

typedef void (*initShimFnType) (RunTimeAbstract *);
typedef void (*deinitShimFnType) ();

class RunTimeBase : public RunTimeAbstractForShim {
public:
        static initShimFnType initShim;
        static deinitShimFnType deinitShim;
        
	RunTimeBase(const RunTimeConfiguration & rtc)
                : _rtc(rtc)
                , _running(false)
                , _load_flow_next(false)
                {}
        virtual ~RunTimeBase() {}
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
	virtual int atomics_style() const { return 1; }
        virtual void log_snapshot() = 0;
        virtual void log_snapshot_guard(const char * guardname) = 0;
        virtual void getPluginNames(std::vector<std::string> & result) = 0;
protected:
	const RunTimeConfiguration   _rtc;
        bool                         _running;
        bool                         _load_flow_next;
};

// list of runtime factory functions:
extern RunTimeBase * _create_classic_runtime(const RunTimeConfiguration &);
extern RunTimeBase * _create_melding_runtime(const RunTimeConfiguration &);
extern RunTimeAbstract * _create_lockfree_runtime(const RunTimeConfiguration &);

} // namespace oflux

#endif // _OFLUX_RUNTIME_BASE_H
