#ifndef _OFLUX_MELDING_RUNTIME_H
#define _OFLUX_MELDING_RUNTIME_H

#include "OFluxRunTime.h"

namespace oflux {
namespace runtime {
namespace melding {

class RunTime : public classic::RunTime {
public:
	friend class RunTimeThread;
	friend class UnlockRunTime;
	RunTime(const RunTimeConfiguration & rtc)
                : classic::RunTime(rtc)
                {}
        virtual ~RunTime() {}
        virtual classic::RunTimeThread * new_RunTimeThread(oflux_thread_t tid = 0);
};

class RunTimeThread : public classic::RunTimeThread {
public:
	RunTimeThread(RunTime * rt, oflux_thread_t tid = 0)
                : classic::RunTimeThread(rt,tid)
                { _condition_context_switch = true; }
	virtual ~RunTimeThread() {}
	virtual int execute_detached(EventBaseSharedPtr & ev,
                int & detached_count_to_increment);
};

} // namespace melding
} // namespace runtime
} // namespace oflux

#endif // _OFLUX_MELDING_RUNTIME_H
