#ifndef _OFLUX_XML
#define _OFLUX_XML

/**
 * @file OFluxXML.h
 * @author Mark Pichora
 * This is an XML reader class that understands XML files produced by oflux
 */

#include "OFluxFlow.h"
#include "boost/shared_ptr.hpp"
#include <string>
#include <expat.h>

namespace oflux {

/**
 * @class XMLReaderException
 * @brief exception class for oops-es in parsing etc
 */
class XMLReaderException {
public:
	XMLReaderException(const char * mesg)
		: _mesg(mesg)
		{}
private:
	std::string _mesg;
};

/**
 * @class AddTarget
 * @brief a storage class for keeping a link from a flow node to its target
 * This is necessary to allow forward referencing in the XML file of nodes.
 */
class AddTarget {
public:
	AddTarget(FlowCase *fc, const char * name)
		: _fc(fc)
		, _name(name)
		{}
	/**
	 * @brief links the flow node found in parsed flow to a case
	 * @param f  the fully parsed flow
	 **/
	void execute(Flow * f);
private:
	FlowCase *  _fc;
	std::string _name;
};

/**
 * @class SetErrorHandler
 * @brief
 * This is necessary to allow forward referencing in the XML file of
 * error handling nodes.
 */
class SetErrorHandler {
public:
	SetErrorHandler(FlowNode *fn, const char * name)
		: _fn(fn)
		, _name(name)
		{}
	/**
	 * @brief links the error flow node found in parsed flow to a node
	 * @param f  the fully parsed flow
	 **/
	void execute(Flow * f);
private:
	FlowNode *  _fn;
	std::string _name;
};

/**
 * @class XMLReader
 * @brief reads an XML flux file given the mappings needed to compile a flow
 * The mappings are used to build a flow with links to the event and
 * conditional function mappings needed to run the flow in the run time.
 */
class XMLReader {
public:
	XMLReader(const char * filename, FlowFunctionMaps *fmaps, const char * pluginxmldir);

	/**
	 * @brief read a file
	 *
	 * @param filename  is the file that is being read
	 *
	 **/
	void read(const char * filename, const char * pluginxmldir);

	static void startMainHandler(void *data, const char *el, const char **attr);
	static void endMainHandler(void *data, const char *el);

	static void startPluginHandler(void *data, const char *el, const char **attr);
	static void endPluginHandler(void *data, const char *el);

	static void dataHandler(void *data, const char *xml_data, int len);
	static void commentHandler(void *data, const char *comment);

	/**
	 * @brief get the result of a successfull reading
	 *
	 * @return smart pointer to the (heap allocated) result flow
	 **/
	Flow * flow() { return _flow; }
	FlowPlugin * flow_plugin() { return _flow_plugin; }
    void new_flow_plugin(const char * external_node, const char * condition, const char * begin_node) 
    {
        _flow_plugin = new FlowPlugin(external_node, condition, begin_node);
        _plugins.insert(std::make_pair(external_node, _flow_plugin));
    }
    void add_flow_plugin() { _flow_plugin = NULL; }
    void add_plugin_node() 
    {
        _flow_plugin->add(_flow_node);
        _flow_node = NULL;
        _flow_successor_list = NULL;
    }

	FlowCase * flow_case() { return _flow_case; }
	void new_flow_case(const char * targetnodename, bool is_virtual);
	void add_flow_case() { _flow_successor->add(_flow_case); _flow_case = NULL; }
	void new_flow_successor() { _flow_successor = new FlowSuccessor(); }
	void add_flow_successor() { _flow_successor_list->add(_flow_successor); _flow_successor = NULL; }
	void new_flow_successor_list() { }
	void add_flow_successor_list() { }
	void new_flow_guard(const char * name, int magicnumber, AtomicMapAbstract * amap)
		{ _flow->addGuard(new FlowGuard(amap,name,magicnumber)); }
	void new_flow_guard_reference(FlowGuard * fg, int unionnumber, int wtype)
		{ 
			_flow_guard_reference = new FlowGuardReference(fg,wtype); 
			_flow_guard_ref_unionnumber = unionnumber;
			_flow_guard_ref_args.clear();
		}
	void complete_flow_guard_reference()
		{
			const char * name = _flow_guard_reference->getName().c_str();
			GuardTransFn guardfn = fmaps()->lookup_guard_translator(name, _flow_guard_ref_unionnumber, _flow_guard_ref_args);
			// assert(guardfn != NULL); // allowed now
			_flow_guard_reference->setGuardFn(guardfn);
			_flow_node->addGuard(_flow_guard_reference);
			_flow_guard_reference = NULL;
		}
	void flow_guard_ref_add_argument(int an)
		{ _flow_guard_ref_args.push_back(an); }
	FlowNode * flow_node() { return _flow_node; }
	void new_flow_node(const char * name, CreateNodeFn createfn, 
			bool is_error_handler, 
			bool is_src,
			bool is_detached,
            bool is_virtual) 
	{ 
		_flow_node = new FlowNode(name,createfn,
				is_error_handler,is_src,is_detached, is_virtual); 
		_flow_successor_list = &(_flow_node->successor_list());
	}
	void add_flow_node();
	void setErrorHandler(const char * err_node_name)
	{
		SetErrorHandler seh(_flow_node,err_node_name);
		_set_error_handlers.push_back(seh);
	}
	FlowFunctionMaps * fmaps() { return _fmaps; }
	FlowFunctionMaps * plugin_fmaps() { return _plugin_fmaps; }
protected:
	/**
	 * @brief just connect the forward flow node references stored up
	 */
    void readdir(const char * plugindir);
    void readfile(const char * filename, XML_StartElementHandler startHandler, XML_EndElementHandler endHandler);
    void loadplugin(const char * filename);

	void finalize();
private:
	FlowFunctionMaps *           _fmaps;
	FlowFunctionMaps *           _plugin_fmaps;
	Flow *                       _flow;
    FlowPlugin *                 _flow_plugin;
	FlowNode *                   _flow_node;
	FlowSuccessorList *          _flow_successor_list;
	FlowSuccessor *              _flow_successor;
	FlowCase *                   _flow_case;
	FlowGuardReference *         _flow_guard_reference;
	int                          _flow_guard_ref_unionnumber;
	std::vector<int>             _flow_guard_ref_args;
	std::vector<AddTarget>       _add_targets;
	std::vector<SetErrorHandler> _set_error_handlers;

    typedef std::multimap<std::string, FlowPlugin *> PluginMap;
    PluginMap _plugins;
};

};



#endif // _OFLUX_XML
