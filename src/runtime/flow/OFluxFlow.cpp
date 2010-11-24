#include <string.h>
#include "flow/OFluxFlow.h"
#include "flow/OFluxFlowGuard.h"
#include "flow/OFluxFlowNode.h"
#include "flow/OFluxFlowLibrary.h"
#include "flow/OFluxFlowCommon.h"
#include "OFluxLogging.h"
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <cerrno>

/**
 * @author Mark Pichora
 * @brief Flow implementation
 *   A flow is the resident data structure that captures the flow of the 
 *   program (it is read from an XML input to the oflux runtime)
 */

namespace oflux {
namespace flow {


Flow::~Flow() 
{ 
        delete_map_contents<std::string,Node>(_nodes); 
        delete_map_contents<std::string,Guard>(_guards); 
        if(!_gaveup_libraries) {
		delete_vector_contents_reversed<Library>(_libraries);
	}
}

void
Flow::log_snapshot_guard(const char * guardname)
{
	std::map<std::string, Guard *>::iterator itr = _guards.find(guardname);
	(*itr).second->log_snapshot();
}

Library * 
Flow::getPrevLibrary(const char * name)
{
        Library * prevlib = NULL;
        for(int i = 0; i <(int)_prev_libraries.size(); i++) {
                if(_prev_libraries[i]->getName() == name) {
                        prevlib = _prev_libraries[i];
                        break;
                }
        }
	return prevlib;
}

bool 
Flow::haveLibrary(const char * name)
{
        bool fd = false;
        for(int i = 0; i <(int)_libraries.size(); i++) {
                if(_libraries[i]->getName() == name) {
                        fd = true;
                        break;
                }
        }
        oflux_log_debug("Flow::haveLibrary() %s, returns %s\n"
                , name
                , fd ? "true" : "false");
        return fd;
}

void 
Flow::log_snapshot()
{
        std::map<std::string, Node *>::iterator mitr = _nodes.begin();
        while(mitr != _nodes.end()) {
                (*mitr).second->log_snapshot();
                mitr++;
        }
}

void 
Flow::pretty_print()
{
        for(int i=0; i < (int) _sources.size(); i++) {
                std::set<std::string> visited;
                _sources[i]->pretty_print(0,'S',&visited);
        }
        /*
        std::map<std::string, Node *>::iterator mitr = _nodes.begin();
        while(mitr != _nodes.end()) {
                (*mitr).second->pretty_print(0);
                mitr++;
        }
        */
}

void
Flow::add(Node *fn)
{
	std::pair<std::string,Node *> pr(fn->getName(),fn);
	_nodes.insert(pr);
	if(fn->getIsSource()) {
		_sources.push_back(fn);
	}
	if(fn->getIsDoor()) {
		_doors.push_back(fn);
	}
}

void
Flow::add(Guard *fg)
{
	_guards[fg->getName()] = fg;
}

void
Flow::turn_off_sources()
{
	for(int i = 0; i < (int)_sources.size(); i++) {
		_sources[i]->turn_off_source();
	}
	_sources.clear();
}

bool
Flow::has_instances()
{
	bool res = false;
	std::map<std::string, Node *>::iterator mitr = _nodes.begin();
	while(mitr != _nodes.end() && !res) {
		res = res || ((*mitr).second->instances() > 0);
		mitr++;
	}
	return res;
}

void 
Flow::assignMagicNumbers() 
{ 
        std::map<std::string,Guard *>::iterator itr = _guards.begin();
        while(itr != _guards.end()) {
                _magic_sorter.addPoint((*itr).second->getName().c_str());
                itr++;
        }
        _magic_sorter.numberAll(); 
        itr = _guards.begin();
        while(itr != _guards.end()) {
                oflux_log_debug("flow assigned guard %s magic no %d\n", (*itr).second->getName().c_str(), (*itr).second->magic_number());
                itr++;
        }
        std::map<std::string, Node *>::iterator nitr = _nodes.begin();
        while(nitr != _nodes.end()) {
                (*nitr).second->sortGuards();
                nitr++;
        }
}


MagicNumberable * GuardMagicSorter::getMagicNumberable(const char * c)
{ 
        std::string cs(c); 
        return _flow->get<Guard>(cs); 
}

Flow::Flow(const Flow & existing_flow, const std::string & name)
	: _name(name)
	, _magic_sorter(this)
{
	for(size_t i = 0; i < existing_flow._libraries.size(); ++i) {
		_prev_libraries.push_back(existing_flow._libraries[i]);
	}
	existing_flow._gaveup_libraries = true;
}

void
Flow::getLibraryNames(std::vector<std::string> & result)
{
        for(size_t i = 0; i < _libraries.size(); i++ ) {
                if(_libraries[i]) {
                        result.push_back(_libraries[i]->getName());
                }
        }
}

void 
Flow::addLibrary(Library * lib, void *init_plugin_params)
{
        _libraries.push_back(lib);
        oflux_log_debug("Flow::addLibrary() %s\n"
                , lib->getName().c_str());
	// init plugin
	if(!getPrevLibrary(lib->getName().c_str())) {
		lib->init(init_plugin_params);
	}
}

void 
Flow::drainGuardsOfEvents()
{
        std::map<std::string,Guard *>::iterator gitr = _guards.begin();
        while(gitr != _guards.end()) {
                (*gitr).second->drain();
                gitr++;
        }
}

} // namespace flow
} // namespace oflux
