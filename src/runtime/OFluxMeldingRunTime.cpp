#include "OFluxMeldingRunTime.h"
#include "atomic/OFluxAtomicHolder.h"
#include "flow/OFluxFlow.h"
#include "flow/OFluxFlowCase.h"
#include "flow/OFluxFlowNode.h"
#include "event/OFluxEventBase.h"
#include "OFluxLogging.h"
#include "OFluxProfiling.h"

namespace oflux {
namespace runtime {
namespace melding {


classic::RunTimeThread * RunTime::new_RunTimeThread(oflux_thread_t tid)
{
        return new RunTimeThread(this,tid);
}


int RunTimeThread::execute_detached(
	  EventBaseSharedPtr & ev
        , int & detached_count_to_increment)
{
        SetTrue st(_detached);
        Increment incr(detached_count_to_increment,_tid);
        _rt->wake_another_thread();
#ifdef PROFILING
        TimerPause oflux_tp(_timer_list);
#endif
        std::vector<flow::Case *> fsuccessors;
        int return_code;
        {
                UnlockRunTime urt(_rt);
                bool loop_on_one = true;
                while(loop_on_one) {
                        return_code = ev->execute();
                        OutputWalker ev_ow = ev->output_type();
                        void * ev_output = ev_ow.next();
                        bool has_no_more_output = ev_ow.next() == NULL;
                        if(!has_no_more_output) {
                                break;
                        }
                        fsuccessors.clear();
                        ev->flow_node()->get_successors(fsuccessors, ev_output, return_code);
                        flow::Case * fsucc_first_case = (fsuccessors.size() == 1 ? fsuccessors[0] : NULL);
                        flow::Node * fsucc_first = (fsucc_first_case ? fsucc_first_case->targetNode() : NULL);
                        if(fsucc_first != NULL 
                                        && fsucc_first->guards().size() == 0
                                        && fsucc_first->getIsDetached()
                                        && ev->atomics().number() == 0) {
                                flow::IOConverter * iocon = fsucc_first_case->ioConverter();
                                CreateNodeFn createfn = fsucc_first->getCreateFn();
                                oflux_log_info("melding two detached nodes %s "
					"-> %s (thread " 
					PTHREAD_PRINTF_FORMAT
					")\n", 
                                        _flow_node_working->getName(),
                                        fsucc_first->getName(),
                                        oflux_self());
                                EventBasePtr new_ev =
                                        ( fsucc_first->getIsSource()
                                        ? (*createfn)(EventBase::no_event_shared,NULL,fsucc_first)
                                        : (*createfn)(ev,iocon->convert(ev_output),fsucc_first)
                                        );
                                new_ev->error_code(return_code);
                                ev = new_ev;
                                _flow_node_working = ev->flow_node();
                        } else {
                                loop_on_one = false;
                        }
                }
                incr.release(_tid);
        }
        return return_code;
}

} // namespace melding
} // namespace runtime
RunTimeBase * _create_melding_runtime(const RunTimeConfiguration & rtc)
{
        return new runtime::melding::RunTime(rtc);
}
} // namespace oflux
