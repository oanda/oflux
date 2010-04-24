#ifndef OFLUX_RUNTIME_ABSTRACT_H
#define OFLUX_RUNTIME_ABSTRACT_H

namespace oflux {

namespace flow {
 class FunctionMaps;
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
	void (*initAtomicMapsF)(int);
};

struct EnvironmentVar {
public:
	EnvironmentVar(int = 0);

	bool nostart;
	int  runtime_number;
};

class RunTimeAbstract {
public:
	virtual ~RunTimeAbstract() {}

	virtual void start() = 0;

	virtual void soft_kill() = 0;

	virtual void hard_kill() = 0;

	virtual void soft_load_flow() = 0;

	virtual void log_snapshot() = 0;

	virtual void log_snapshot_guard(const char * guardname) = 0;
};

namespace runtime {

class Factory {
public:
	enum {    classic  = 0
		, melding  = 1
		, lockfree = 4 
	};

	static RunTimeAbstract * create(int type, const RunTimeConfiguration & rtc);
};


} // namespace runtime

} // namespace oflux



#endif // OFLUX_RUNTIME_ABSTRACT_H
