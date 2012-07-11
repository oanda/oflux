#ifndef _OFLUX_FLOW
#define _OFLUX_FLOW
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

/**
 * @file OFluxFlow.h
 * @author Mark Pichora
 * Top-level flow data structure which encapsulates the program data flow.
 * This only changes if we have the server load a new flow graph.
 */


#include "OFluxOrderable.h"
#include <map>
#include <vector>
#include <string>

namespace oflux {
namespace flow {

class Flow;
class Node;
class Guard;
class Library;

class GuardMagicSorter : public MagicSorter {
public:
        GuardMagicSorter(Flow * f) : _flow(f) {}
        virtual MagicNumberable * getMagicNumberable(const char * c);
private:
        Flow * _flow;
};

class Library;
class FlowHolder;
/**
 * @class Flow
 * @brief a flow is a collection of flow nodes (connected internally)
 */
class Flow {
public:
	typedef FlowHolder ParentObjType;

        Flow(const std::string & name) 
		: _name(name)
		, _magic_sorter(this) 
		, _gaveup_libraries(false)
	{}
        Flow(const Flow &, const std::string & name);
        ~Flow();

        /**
         * @brief get a vector of sources in the flow
         * @return a vector of sources
         **/
        std::vector<Node *> & sources() { return _sources; }
        std::vector<Node *> & doors() { return _doors; }
        std::map<std::string, Node *> & nodes() { return _nodes; }

        /**
         * @brief add a flow node
         * @param fn Node to add to the flow
         **/
        void add(Node *fn);

	template<typename T>
	inline T* get(const std::string & n)
	{
		return NULL;
	}

	inline const std::string & name() const { return _name; }

        /**
         * @brief add a guard to the guard map (internal) for lookup
         * @param fg the guard to add to this flow
         */
        void add(Guard *fg);
        /**
         * @brief output to the logger some information about the state in
         * this object(flow)
         */
        void log_snapshot();
        void log_snapshot_guard(const char * gname);
        /**
         * @brief log a "pretty printed" flow DAG (graph) showing the flow
         */
        void pretty_print();
        /**
         * @brief take the source property away from the node in this flow
         *   that currently have it (they will no longer spawn themselves)
         */
        void turn_off_sources();
        /**
         * @brief determine whether there are event instances using this flow
         */
        bool has_instances();
        /**
         * @brief add a library to this flow
         */
        void addLibrary(Library * lib, void * init_plugin_params);
        /**
         * @brief determine if this library is already know to this flow
         */
        bool haveLibrary(const char * name);
	Library * getPrevLibrary(const char * name);
        /**
         * @brief return the names of the plugin libraries in loaded order
         */
        void getLibraryNames(std::vector<std::string> & result);
        /**
         * @brief [internal use] -- total ordering of guards is determined
         */
        void assignMagicNumbers();
        /**
         * @brief add an ordering constraint on the acquisition of two
         *   guards in this flow
         */
        inline void addGuardPrecedence(const char * before, const char * after)
        { _magic_sorter.addInequality(before,after); }
        void drainGuardsOfEvents();
	int sources_count() const { return _sources.size(); }
	int doors_count() const { return _doors.size(); }
private:
	std::string                    _name;
        std::map<std::string, Node *>  _nodes;
        std::vector<Node *>            _sources;
        std::vector<Node *>            _doors;
        std::map<std::string, Guard *> _guards;
        GuardMagicSorter               _magic_sorter;
        std::vector<Library *>         _libraries;
        std::vector<Library *>         _prev_libraries;
	mutable bool                   _gaveup_libraries;
};

/**
 * @brief obtain a flow node by name
 * @param name given a name of a flow node
 * @return the flow node of that name (NULL for not found)
 **/
template<>
inline Node * Flow::get<Node>(const std::string & name)
{
	std::map<std::string, Node *>::iterator itr = _nodes.find(name);
	return (itr == _nodes.end() ? NULL : (*itr).second);
}

/**
 * @brief lookup a guard with a particular name
 * @param name of the guard you are looking for
 * @return a pointer to the guard (NULL if not found)
 */
template<>
inline Guard * Flow::get<Guard>(const std::string & name)
{
	std::map<std::string, Guard *>::iterator itr = _guards.find(name);
	return (itr == _guards.end() ? NULL : (*itr).second);
}


class FlowHolder {
public:
	struct ParentObjType {
		void add(FlowHolder *) {}
	};

	FlowHolder()
		: _flow(NULL)
	{}
	Flow * flow() { return _flow; }
	void add(Flow * f) { _flow = f; }
private:
	Flow * _flow;
}; 


} // namespace flow

} // namespace oflux

#endif // _OFLUX_FLOW

