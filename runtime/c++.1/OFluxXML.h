#ifndef _OFLUX_XML
#define _OFLUX_XML

/**
 * @file OFluxXML.h
 * @author Mark Pichora
 * This is an XML reader class that understands XML files produced by oflux
 */

#include "OFluxFlow.h"
//#include "boost/shared_ptr.hpp"
#include <string>

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
	XMLReader(const char * filename, FlowFunctionMaps *fmaps);

	/**
	 * @brief read a file
	 *
	 * @param filename  is the file that is being read
	 *
	 **/
	void read(const char * filename);

	static void startHandler(void *data, const char *el, const char **attr);
	static void endHandler(void *data, const char *el);
	static void dataHandler(void *data, const char *xml_data, int len);
	static void commentHandler(void *data, const char *comment);

	/**
	 * @brief get the result of a successfull reading
	 *
	 * @return smart pointer to the (heap allocated) result flow
	 **/
	Flow * flow() { return _flow; }
	FlowCase * flow_case() { return _flow_case; }
	void new_flow_case(const char * targetnodename) 
	{ 
		_flow_case = new FlowCase(NULL); 
		AddTarget at(_flow_case,targetnodename);
		_add_targets.push_back(at);
	}
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
			assert(guardfn != NULL);
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
			bool is_detached) 
	{ 
		_flow_node = new FlowNode(name,createfn,
				is_error_handler,is_src,is_detached); 
		_flow_successor_list = &(_flow_node->successor_list());
	}
	void add_flow_node() 
	{ 
		_flow->add(_flow_node);
		_flow_node = NULL;
		_flow_successor_list = NULL;
	}
	void setErrorHandler(const char * err_node_name)
	{
		SetErrorHandler seh(_flow_node,err_node_name);
		_set_error_handlers.push_back(seh);
	}
	FlowFunctionMaps * fmaps() { return _fmaps; }
protected:
	/**
	 * @brief just connect the forward flow node references stored up
	 */
	void finalize();
private:
	FlowFunctionMaps *           _fmaps;
	Flow *                       _flow;
	FlowNode *                   _flow_node;
	FlowSuccessorList *          _flow_successor_list;
	FlowSuccessor *              _flow_successor;
	FlowCase *                   _flow_case;
	FlowGuardReference *         _flow_guard_reference;
	int                          _flow_guard_ref_unionnumber;
	std::vector<int>             _flow_guard_ref_args;
	std::vector<AddTarget>       _add_targets;
	std::vector<SetErrorHandler> _set_error_handlers;
};

};



#endif // _OFLUX_XML
