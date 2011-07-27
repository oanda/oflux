#ifndef OFLUX_RUNTIME_CONFIGURATION_H
#define OFLUX_RUNTIME_CONFIGURATION_H

namespace oflux {

class PluginSourceAbstract {
public:
	virtual ~PluginSourceAbstract() {}
	virtual const char * nextXmlFile() const = 0; 
		// NULL indicates its at the end
		// XML filenames are made available here
};

class DirPluginSource : public PluginSourceAbstract {
public:
	DirPluginSource(const char * plugin_dir);
	virtual ~DirPluginSource();
	virtual const char * nextXmlFile() const;
private:
	void * _hidden_vector;
	int _index;
};

class SimpleCharArrayPluginSource : public PluginSourceAbstract {
public:
	SimpleCharArrayPluginSource(const char * plugin_names[]);
	virtual const char * nextXmlFile() const;
private:
	const char * * _pnames;
};

template< typename StdContainerOfStdString >
class SimpleCharArrayPluginSource : public PluginSourceAbstract {
public:
	SimpleCharArrayPluginSource(StdContainerOfStdString & cont)
		: _cont(cont)
		, _itr(cont.begin())
		, _itr_end(cont.end())
	{}
	virtual const char * nextXmlFile() const
	{
		if(_itr == _end_itr) {
			return NULL;
		}
		const char * res = (*_itr).c_str();
		++_itr;
		return res;
	}
private:
	StdContainerOfStdString & _cont;
	typename StdContainerOfStdString::const_iterator _itr;
	typename StdContainerOfStdString::const_iterator _end_itr;
};

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
	flow::FunctionMapsAbstract * flow_maps;
	const PluginSourceAbstract * plugin_name_source;
	const char * plugin_lib_dir;    // plugin lib directory
	void * init_plugin_params;
	void (*initAtomicMapsF)(int);
	const char * doors_dir;
};

} // namespace oflux

#endif  // OFLUX_RUNTIME_CONFIGURATION_H
