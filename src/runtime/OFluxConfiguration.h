#ifndef OFLUX_RUNTIME_CONFIGURATION_H
#define OFLUX_RUNTIME_CONFIGURATION_H
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

namespace oflux {
namespace flow {
 class FunctionMapsAbstract;
} // namespace flow

class PluginSourceAbstract {
public:
	virtual ~PluginSourceAbstract() {}
	virtual const char * nextXmlFile() = 0; 
		// NULL indicates its at the end
		// XML filenames are made available here
		// accessible relative paths or full paths should be acceptable
	virtual void reset() = 0;
};

class DirPluginSource : public PluginSourceAbstract {
public:
	DirPluginSource(const char * plugin_dir);
	virtual ~DirPluginSource();
	virtual const char * nextXmlFile();
	virtual void reset() { _index = 0; }
private:
	void * _hidden_vector;
	int _index;
};

class SimpleCharArrayPluginSource : public PluginSourceAbstract {
public:
	SimpleCharArrayPluginSource(const char * plugin_names[]);
	virtual const char * nextXmlFile();
private:
	const char * * _pnames_orig;
	const char * * _pnames;
};

template< typename StdContainerOfStdString >
class StdContainerPluginSource : public PluginSourceAbstract {
public:
	StdContainerPluginSource(StdContainerOfStdString & cont)
		: _cont(cont)
		, _itr(cont.begin())
		, _itr_end(cont.end())
	{}
	virtual const char * nextXmlFile()
	{
		if(_itr == _itr_end) {
			return 0;
		}
		const char * res = (*_itr).c_str();
		++_itr;
		return res;
	}
	virtual void reset() { _itr = _cont.begin(); }
private:
	StdContainerOfStdString & _cont;
	typename StdContainerOfStdString::const_iterator _itr;
	typename StdContainerOfStdString::const_iterator _itr_end;
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
	PluginSourceAbstract * plugin_name_source;
	const char * plugin_lib_dir;    // plugin lib directory
	void * init_plugin_params;
	void (*initAtomicMapsF)(int);
	const char * doors_dir;
};

} // namespace oflux

#endif  // OFLUX_RUNTIME_CONFIGURATION_H
