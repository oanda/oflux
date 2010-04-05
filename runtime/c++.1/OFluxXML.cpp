#include <string.h>
#include "OFluxXML.h"
#include "OFluxFlow.h"
#include "OFluxFlowNode.h"
#include "OFluxFlowCase.h"
#include "OFluxFlowGuard.h"
#include "OFluxFlowLibrary.h"
#include "OFluxFlowFunctions.h"
#include "OFluxLogging.h"
#include <vector>
#include <set>
#include <expat.h>
#include <fstream>
#include <cassert>
#include <dirent.h>

#define XML_READER_MAX_LINE 2048


namespace oflux {
namespace xml {


class Reader;

/**
 * @class AddTarget
 * @brief a storage class for keeping a link from a flow node to its target
 * This is necessary to allow forward referencing in the XML file of nodes.
 */
class AddTarget {
public:
        AddTarget(flow::Case * fc
		, const char * name
		, int node_output_unionnumber
		, Reader * xmlreader
		, const std::string & scope_name)
        : _fc(fc)
        , _name(name)
        , _node_output_unionnumber(node_output_unionnumber)
        , _target_input_unionnumber(0)
        , _xmlreader(xmlreader)
	, _scope_name(scope_name)
        {}
        /**
         * @brief links the flow node found in parsed flow to a case
         * @param f  the fully parsed flow
         **/
        void execute(flow::Flow * f);
private:
        flow::Case * _fc;
        std::string  _name; //target
        int          _node_output_unionnumber;
        int          _target_input_unionnumber;
        Reader *     _xmlreader;
	std::string  _scope_name;
};


/**
 * @class SetErrorHandler
 * @brief
 * This is necessary to allow forward referencing in the XML file of
 * error handling nodes.
 */
class SetErrorHandler {
public:
        SetErrorHandler(flow::Node *fn, const char * name)
                : _fn(fn)
                , _name(name)
        {}
        /**
         * @brief links the error flow node found in parsed flow to a node
         * @param f  the fully parsed flow
         **/
        void execute(flow::Flow * f);
private:
        flow::Node * _fn;
        std::string  _name;
};

class DependencyTracker {
public:
        DependencyTracker() {}
        void addDependency(const char * fl);
        bool isDependency(const char * fl);
protected:
        void canonize(std::string & filename); // modify in place
private:
        std::set<std::string> _depends_set;
};


class ScopedFunctionMaps {
public:
	typedef std::set<flow::FunctionMaps *> Context;
	class Scope {
	public:
		Scope(Context & c)
			: _c(c)
		{}

		CreateNodeFn
		lookup_node_function(const char *n);

		ConditionFn
		lookup_conditional(
			  const char * n
			, int argno
			, int unionnumber);

		GuardTransFn
		lookup_guard_translator(
			  const char * guardname
			, int union_number
			, const char * hash
			, int wtype
			, bool late);

		AtomicMapAbstract *
		lookup_atomic_map(const char * guardname);

		FlatIOConversionFun
		lookup_io_conversion(int from_unionnumber, int to_unionnumber);
	private:
		Context & _c;
	};

	boost::shared_ptr<Scope>
	get(const char * scopename) {
		std::map<std::string, Context>::iterator itr =
			_map.find(scopename);
		assert(itr != _map.end());
		return boost::shared_ptr<Scope>(new Scope(itr->second));
	}

	void
	add(
		  const char * scopename
		, flow::FunctionMaps * fmaps
		, const std::vector<std::string> & deps);
private:
	static void
	_addIntoContext(
		  Context & c1
		, const Context & c2)
	{
		Context::iterator itr = c2.begin();
		while(itr != c2.end()) {
			c1.insert(*itr);
			++itr;
		}
	}
private:
	std::map< std::string, Context > _map;
};

class XMLVocab {
public:
	static const char * attr_name;
	static const char * attr_argno;
	static const char * attr_nodetarget;
	static const char * attr_source;
	static const char * attr_isnegated;
	static const char * attr_iserrhandler;
	static const char * attr_detached;
	static const char * attr_unionnumber;
	static const char * attr_inputunionnumber;
	static const char * attr_outputunionnumber;
	static const char * attr_after;
	static const char * attr_before;
	static const char * attr_late;
	static const char * attr_hash;
	static const char * attr_wtype;
	static const char * attr_function;
	static const char * attr_external;
	static const char * attr_ofluxversion;

	static const char * element_flow;
	static const char * element_plugin;
	static const char * element_library;
	static const char * element_depend;
	static const char * element_guard;
	static const char * element_guardprecedence;
	static const char * element_guardref;
	static const char * element_argument;
	static const char * element_add;
	static const char * element_remove;
	static const char * element_node;
	static const char * element_successorlist;
	static const char * element_successor;
	static const char * element_errorhandler;
	static const char * element_case;
	static const char * element_condition;
};

class Attribute {
public:
	Attribute()
		: _v(NULL)
	{}
	Attribute(const char *v)
		: _v(v)
	{}
	Attribute(const Attribute & a)
		: _v(a._v)
	{}

	Attribute & operator=(const Attribute & a)
	{
		_v = a._v;
		return *this;
	}
	int intVal() const { return atoi(_v); }
	const char * c_str() const { return _v; }
	bool boolVal() const
	{
		static std::string t = "true";
		return t == _v;
	}
private:
	const char * _v;
};



class AttributeMap : public std::map<const char *,Attribute> {
public:
	Attribute & getOrThrow(const char * k)
	{
		std::map<const char *,Attribute>::iterator itr = find(k);
		if(itr == end()) {
			std::string ex_msg = "unfound attribute key ";
			ex_msg += k;
			throw ReaderException(ex_msg.c_str());
		}
		return (*itr).second;
	}
};

//template<typename Child >
//void pc_add(Child::ParentObjType *, Child *, Reader &);

class Reader;

template<typename Element>
Element * flow_element_factory(AttributeMap &,Reader &);

class ReaderStateAbstract {
public:
	virtual ~ReaderStateAbstract() {}
	virtual ReaderStateAbstract * parent() = 0;
	virtual const char * elementName() const = 0;
	virtual ReaderStateAbstract * self() { return this; }; // dereferencing
};

class ReaderStateBasic : public ReaderStateAbstract {
public:
	ReaderStateBasic(ReaderStateAbstract * prs)
		: _prs(prs)
	{}
	virtual ~ReaderStateBasic() {}
	virtual ReaderStateAbstract * parent() { return _prs; }
	virtual const char * elementName() const { return NULL; }
protected:
	ReaderStateAbstract * _prs;
};

class ReaderStateGuardPrecedence : public ReaderStateBasic {
public:
	static ReaderStateAbstract * factory(
		  const char *
		, AttributeMap & amap
		, Reader & reader );

	ReaderStateGuardPrecedence(ReaderStateAbstract * prs)
		: ReaderStateBasic(prs)
	{}
	virtual ~ReaderStateGuardPrecedence() {}
	virtual const char * elementName() const { return XMLVocab::element_guardprecedence; }
};

class ReaderStateDepend : public ReaderStateBasic {
public:
	static ReaderStateAbstract * factory(
		  const char *
		, AttributeMap & amap
		, Reader & reader );

	ReaderStateDepend(ReaderStateAbstract * prs)
		: ReaderStateBasic(prs)
	{}
	virtual ~ReaderStateDepend() {}
	virtual const char * elementName() const { return XMLVocab::element_depend; }
};

class TerminalReaderState : public ReaderStateAbstract {
public:
	typedef flow::FlowHolder ObjType;

	TerminalReaderState() {}
	virtual ~TerminalReaderState() {}
	flow::Flow * flow() { return _fh->flow(); }
	virtual ReaderStateAbstract * parent() { return NULL; }
	virtual const char * elementName() const { return ""; }
private:
	flow::FlowHolder * _fh;
};

class ReaderStateRemoval : public ReaderStateAbstract {
public:
	static ReaderStateAbstract * factory(
		  const char *
		, AttributeMap &
		, Reader & reader )
	{
		throw ReaderException("ReaderStateRemoval not implemented : <remove> tag");
		return NULL;
	}
	ReaderStateRemoval() {}
	virtual ~ReaderStateRemoval() {}
	virtual const char * elementName() const
	{ return XMLVocab::element_remove; }
	virtual ReaderStateAbstract * self() { return NULL; }
	virtual ReaderStateAbstract * parent() { return NULL; }
};


template<typename T>
struct ElementName {
	static const char * name;
};

template< typename ThisType >
class ReaderState;

class Reader {
public:

	static const char * currentDir;

	// primary interface:
        Reader(   flow::FunctionMaps *fmaps
		, const char * pluginxmldir
		, const char * pluginlibdir
		, void * initpluginparams
		, const flow::Flow * existing_flow);

	flow::Flow * read( const char * filename );

	// internal stuff:
	void readPluginIfUnread(const char * plugin_name)
	{
		std::string depxml = _plugin_xml_dir;
		depxml += "/";
		depxml += plugin_name;
		depxml += ".xml";
		flow::Flow * flow = get<flow::Flow>();
		if(!flow->haveLibrary(plugin_name)) {
			if(_depends_visited.isDependency(depxml.c_str())) {
				depxml += " circular dependency -- already loading";
				throw ReaderException(depxml.c_str());
			}
			_depends_visited.addDependency(depxml.c_str());
			ReaderStateAbstract * preserve = _reader_state;
			while(_reader_state && _reader_state->elementName()
					!= XMLVocab::element_flow) {
				_reader_state = _reader_state->parent();
			}
			if(!_reader_state  || _reader_state->elementName()
                                        != XMLVocab::element_flow) {
				throw ReaderException("readPluginIfUnread() failed to find flow reader state");
			}
			readxmlfile(depxml.c_str());
			_reader_state = preserve;
		}
	}
	void addLibrary(flow::Library * l, flow::FunctionMaps * fms)
	{
		_scope_name = l->getName();

		_scoped_fmaps.add(
			  l->getName().c_str()
			, fms
			, l->getDependencies());
		get<flow::Flow>()->addLibrary(l,_init_plugin_params);
	}
	void setErrorHandler(const char * eh_name)
	{
		if(allow_addition()) {
			flow::Node * node = get<flow::Node>();
			SetErrorHandler seh(node,eh_name);
			_set_error_handlers.push_back(seh);
		}
	}
	void addTarget(flow::Case * c, const char * target_name, int out_unionnumber)
	{
                AddTarget at(c,target_name, out_unionnumber, this, _scope_name);
                _add_targets.push_back(at);
	}

	typedef boost::shared_ptr<ScopedFunctionMaps::Scope> ScopePtr;

	ScopePtr fromThisScope(const char * s = NULL)
	{ return _scoped_fmaps.get(s ? s : _scope_name.c_str()); }

	const flow::Flow * existing_flow() const { return _existing_flow; }

	bool allow_addition() { return _allow_addition; }
	void allow_addition(bool a) { _allow_addition = a; }

	template<typename T>
	T * get() // find closest reader state object of that type
	{
		const char * rsa_el_name = ElementName<T>::name;
		ReaderStateAbstract * rsa = _reader_state;
		while(rsa && rsa->elementName() != rsa_el_name) {
			rsa = rsa->parent();
		}
		if(!rsa) return NULL;
		ReaderState<T> * rsptr = reinterpret_cast<ReaderState<T> *>(rsa);
		return rsptr->obj();
	}

	template<typename T>
	T * find(const char * t_name)
	{
		typename T::ParentObjType * pt = get<typename T::ParentObjType>();
		std::string s = t_name;
		return (pt ? pt->get<T>(s) : NULL);
	}

	const char * pluginLibDir() const
	{ return _plugin_lib_dir; }

	ReaderStateAbstract * getState() { return _reader_state; }

protected:
	void pushState( const char * element_str, AttributeMap &);
	ReaderStateAbstract * popState();

	// expat hooks:
	static void startHandler(void *data, const char *el, const char **attr);
	static void endHandler(void *data, const char *el);
        static void dataHandler(void *data, const char *xml_data, int len);
        static void commentHandler(void *data, const char *comment);

	void readxmlfile(const char * filename);
	void readxmldir(flow::FlowHolder *);

private:
	ScopedFunctionMaps _scoped_fmaps;
	std::string _scope_name;
	const char * _plugin_lib_dir;
	const char * _plugin_xml_dir;
	void * _init_plugin_params;
	const flow::Flow * _existing_flow;
	ReaderStateAbstract * _reader_state;
        DependencyTracker _depends_visited;
	bool _allow_addition;
        std::vector<AddTarget> _add_targets;
        std::vector<SetErrorHandler> _set_error_handlers;
};



class ReaderStateAddition : public ReaderStateAbstract {
public:
	static ReaderStateAbstract * factory(
		  const char *
		, AttributeMap &
		, Reader & reader )
	{
		return new ReaderStateAddition(reader);
	}

	ReaderStateAddition(Reader & reader)
		: _rsa(reader.getState())
		, _reader(reader)
		, _old_allow_addition(reader.allow_addition())
	{ reader.allow_addition(true); }

	virtual ~ReaderStateAddition()
	{ _reader.allow_addition(_old_allow_addition); }


	virtual const char * elementName() const
	{ return XMLVocab::element_add; }

	virtual ReaderStateAbstract * self() { return _rsa; } // defer up
	virtual ReaderStateAbstract * parent() { return _rsa; }

private:
	ReaderStateAbstract * _rsa;
	Reader & _reader;
	bool _old_allow_addition;
};

class ReaderStateErrorHandler : public ReaderStateBasic {
public:
	static ReaderStateAbstract * factory(
		  const char *
		, AttributeMap & amap
		, Reader & reader )
	{
		const char * eh_name = amap.getOrThrow(XMLVocab::attr_name).c_str();
		reader.setErrorHandler(eh_name);
		return new ReaderStateErrorHandler(reader.getState());
	}

	ReaderStateErrorHandler(ReaderStateAbstract * prs)
		: ReaderStateBasic(prs)
	{}
	virtual ~ReaderStateErrorHandler() {}
	virtual const char * elementName() const { return XMLVocab::element_errorhandler; }
};

template< typename ThisType >
class ReaderState : public ReaderStateBasic {
public:
	typedef ThisType ObjType;

	static ReaderStateAbstract * factory(
		  const char * element_str
		, AttributeMap & map
		, Reader & reader )
	{
		return new ReaderState(
			  element_str
			, flow_element_factory<ThisType>(map,reader)
			, reader.getState()
			, reader );
	}

	ReaderState( const char * element_str
		   , ThisType * fresh_obj
		   , ReaderStateAbstract * prs
		   , Reader & reader)
		: ReaderStateBasic(prs)
		, _element_name(element_str)
		, _obj(fresh_obj)
		, _reader(reader)
	{}

	virtual ~ReaderState()
	{
		if(!parent_obj()) {
			oflux_log_debug("~ReaderState ditching obj %p for %s %s\n"
				, obj()
				, _element_name
				, _reader.allow_addition()
					? "true" : "false");
			delete obj();
		} else if(obj()) {
			pc_add(parent_obj(),obj(),_reader);
		}
	}

	virtual const char * elementName() const { return _element_name; }

	ThisType * obj() { return _obj; }

	typename ThisType::ParentObjType * parent_obj()
	{
		ReaderStateAbstract * prs = (_prs ? _prs->self() : NULL);
		return  ( prs
			? dynamic_cast<ReaderState<typename ThisType::ParentObjType > * >(prs)->obj()
			: NULL);
	}
private:
	const char * _element_name;
	ThisType * _obj;
	Reader & _reader;
};

typedef ReaderStateAbstract * (*ReaderStateFun)(const char *, AttributeMap & , Reader &);






ReaderStateAbstract *
ReaderStateGuardPrecedence::factory(
		  const char *
		, AttributeMap & amap
		, Reader & reader )
{
	const char * before = amap.getOrThrow(XMLVocab::attr_before).c_str();
	const char * after = amap.getOrThrow(XMLVocab::attr_after).c_str();
	reader.get<flow::Flow>()->addGuardPrecedence(before,after);
	return new ReaderStateGuardPrecedence(reader.getState());
}

ReaderStateAbstract *
ReaderStateDepend::factory(
	  const char *
	, AttributeMap & amap
	, Reader & reader )
{
	const char * name = amap.getOrThrow(XMLVocab::attr_name).c_str();
	reader.readPluginIfUnread(name);
	return new ReaderStateDepend(reader.getState());
}


void
AddTarget::execute(flow::Flow * f)
{
        flow::Node *fsrc = f->get<flow::Node>(_name);
        assert(fsrc);
        _fc->setTargetNode(fsrc);
	oflux_log_debug("AddTarget::execute %p %s\n", _fc,_name.c_str());
        _target_input_unionnumber = fsrc->inputUnionNumber();
        FlatIOConversionFun fiocf =
		_xmlreader->fromThisScope(_scope_name.c_str())->lookup_io_conversion(_node_output_unionnumber, _target_input_unionnumber);
        if(fiocf) {
                assert(_fc->ioConverter() == &flow::IOConverter::standard_converter);
                _fc->setIOConverter(new flow::IOConverter(fiocf));
        }
}

void
SetErrorHandler::execute(flow::Flow * f)
{
        flow::Node *fsrc = f->get<flow::Node>(_name);
        assert(fsrc);
        assert(fsrc->getIsErrorHandler());
        _fn->setErrorHandler(fsrc);
}

void
DependencyTracker::addDependency(const char * fl)
{
        std::string filename = fl;
        canonize(filename);
        _depends_set.insert(filename);
}

bool
DependencyTracker::isDependency(const char * fl)
{
        std::string filename = fl;
        canonize(filename);
        return _depends_set.find(filename) != _depends_set.end();
}

void
DependencyTracker::canonize(std::string & filename)
{
        // just get rid of leading "./"
        if(filename.length() >= 2 && filename[0] == '.' && filename[1] == '/') {
                filename.erase(filename.begin());
                filename.erase(filename.begin());
        }
}

void
ScopedFunctionMaps::add(
	  const char * scopename
	, flow::FunctionMaps * fmaps
	, const std::vector<std::string> & deps)
{
	std::pair<std::string,ScopedFunctionMaps::Context> pr;
	pr.first = scopename;
	pr.second.insert(fmaps);
	std::vector<std::string>::const_iterator itr = deps.begin();
	while(itr != deps.end()) {
		std::map<std::string,Context>::iterator fitr = _map.find(*itr);
		assert(fitr != _map.end());
		_addIntoContext(pr.second,fitr->second);
		++itr;
	}
	_map.insert(pr);
}

CreateNodeFn
ScopedFunctionMaps::Scope::lookup_node_function(const char *n)
{
        CreateNodeFn res = NULL;
	ScopedFunctionMaps::Context::const_iterator itr = _c.begin();
        while(res == NULL && itr != _c.end()) {
                res = (*itr)->lookup_node_function(n);
		++itr;
        }
        return res;
}

ConditionFn
ScopedFunctionMaps::Scope::lookup_conditional(
	  const char * n
	, int argno
	, int unionnumber)
{
        ConditionFn res = NULL;
	ScopedFunctionMaps::Context::const_iterator itr = _c.begin();
        while(res == NULL && itr != _c.end()) {
                res = (*itr)->lookup_conditional(n,argno,unionnumber);
		++itr;
        }
        return res;
}

GuardTransFn
ScopedFunctionMaps::Scope::lookup_guard_translator(
	  const char * guardname
        , int union_number
        , const char * hash
        , int wtype
	, bool late)
{
        GuardTransFn res = NULL;
	ScopedFunctionMaps::Context::const_iterator itr = _c.begin();
        while(res == NULL && itr != _c.end()) {
                res = (*itr)->lookup_guard_translator(
			  guardname
                        , union_number
                        , hash
                        , wtype
			, late);
		++itr;
        }
        return res;
}

AtomicMapAbstract *
ScopedFunctionMaps::Scope::lookup_atomic_map(const char * guardname)
{
        AtomicMapAbstract * res = NULL;
	ScopedFunctionMaps::Context::const_iterator itr = _c.begin();
        while(res == NULL && itr != _c.end()) {
                res = (*itr)->lookup_atomic_map(guardname);
		++itr;
        }
        return res;
}

FlatIOConversionFun
ScopedFunctionMaps::Scope::lookup_io_conversion(
	  int from_unionnumber
	, int to_unionnumber)
{
        FlatIOConversionFun res = NULL;
	ScopedFunctionMaps::Context::const_iterator itr = _c.begin();
        while(res == NULL && itr != _c.end()) {
                res = (*itr)->lookup_io_conversion(
			  from_unionnumber
			, to_unionnumber);
		++itr;
        }
        return res;
}


const char * XMLVocab::attr_name = "name";
const char * XMLVocab::attr_argno = "argno";
const char * XMLVocab::attr_nodetarget = "nodetarget";
const char * XMLVocab::attr_source = "source";
const char * XMLVocab::attr_isnegated = "isnegated";
const char * XMLVocab::attr_iserrhandler = "iserrhandler";
const char * XMLVocab::attr_detached = "detached";
const char * XMLVocab::attr_unionnumber = "unionnumber";
const char * XMLVocab::attr_inputunionnumber = "inputunionnumber";
const char * XMLVocab::attr_outputunionnumber = "outputunionnumber";
const char * XMLVocab::attr_after = "after";
const char * XMLVocab::attr_before = "before";
const char * XMLVocab::attr_late = "late";
const char * XMLVocab::attr_hash = "hash";
const char * XMLVocab::attr_wtype = "wtype";
const char * XMLVocab::attr_function = "function";
const char * XMLVocab::attr_external = "external";
const char * XMLVocab::attr_ofluxversion = "ofluxversion";

const char * XMLVocab::element_flow = "flow";
const char * XMLVocab::element_plugin = "plugin";
const char * XMLVocab::element_library = "library";
const char * XMLVocab::element_depend = "depend";
const char * XMLVocab::element_guard = "guard";
const char * XMLVocab::element_guardprecedence = "guardprecedence";
const char * XMLVocab::element_guardref = "guardref";
const char * XMLVocab::element_argument = "argument";
const char * XMLVocab::element_add = "add";
const char * XMLVocab::element_remove = "remove";
const char * XMLVocab::element_node = "node";
const char * XMLVocab::element_successorlist = "successorlist";
const char * XMLVocab::element_successor = "successor";
const char * XMLVocab::element_errorhandler = "errorhandler";
const char * XMLVocab::element_case = "case";
const char * XMLVocab::element_condition = "condition";

static void
fillAttributeMap(
	  AttributeMap & amap
	, const char ** attr)
{
	static const char * vocab_list[] =
		{ XMLVocab::attr_name
		, XMLVocab::attr_argno
		, XMLVocab::attr_nodetarget
		, XMLVocab::attr_source
		, XMLVocab::attr_isnegated
		, XMLVocab::attr_iserrhandler
		, XMLVocab::attr_detached
		, XMLVocab::attr_unionnumber
		, XMLVocab::attr_inputunionnumber
		, XMLVocab::attr_outputunionnumber
		, XMLVocab::attr_after
		, XMLVocab::attr_before
		, XMLVocab::attr_late
		, XMLVocab::attr_hash
		, XMLVocab::attr_wtype
		, XMLVocab::attr_function
		, XMLVocab::attr_external
		, XMLVocab::attr_ofluxversion
		, NULL
		};
        for(size_t i = 0; attr[i]; i += 2) {
		Attribute attrib(attr[i+1]);
		int fd = -1;
		for(size_t j = 0; vocab_list[j]; ++j) {
			if(strcmp(vocab_list[j],attr[i]) == 0) {
				fd = j;
				break;
			}
		}
		if(fd < 0) {
			std::string ex_msg = "Unknown attribute ";
			ex_msg += attr[i];
			throw ReaderException(ex_msg.c_str());
		}
		amap[vocab_list[fd]] = attrib;
	}
}

//
// a typed stack for building structures from XML:
//

template<typename Child >
void
pc_add(typename Child::ParentObjType *p, Child *c, Reader & reader)
{
	if(reader.allow_addition()) {
		p->add(c);
	}
}

// + specializations that are allowed

template<>
void
pc_add<flow::Node>(
	  flow::Flow * f
	, flow::Node * n
	, Reader & reader)
{
	if(!f->get<flow::Node>(n->getName())) {
		f->add(n);
	}
}

extern "C" {
typedef flow::FunctionMaps * FlowFunctionMapFunction ();
}

template<>
void
pc_add<flow::Library>(
	  flow::Flow * f
	, flow::Library * l
	, Reader & reader)
{
	if(!l->load()) {
		std::string msg = "could not load library : ";
		msg += l->getName();
		throw ReaderException(msg.c_str());
	}
	std::string flowfunctionmapfunction = "flowfunctionmaps__";
	l->addSuffix(flowfunctionmapfunction);
	FlowFunctionMapFunction * ffmpfun =
                l->getSymbol<FlowFunctionMapFunction>(flowfunctionmapfunction.c_str());
	if(!ffmpfun) {
		std::string msg = "could not load flowfunctionmapfunction for library : ";
		msg += l->getName();
		throw ReaderException(msg.c_str());
	}
	reader.addLibrary(l,(*ffmpfun)());
}

template<>
void
pc_add<flow::SuccessorList>(
	  flow::Node * f
	, flow::SuccessorList * sl
	, Reader &)
{
	if(!f->successor_list()) {
		f->successor_list(sl);
	}
}

template<>
void
pc_add<flow::Case>(
	  flow::Successor * s
	, flow::Case * c
	, Reader & reader)
{
	if(reader.allow_addition()) {
		bool is_front = false;
		ReaderStateAbstract * rsa = reader.getState();
		while(rsa) {
			if(rsa->elementName() == XMLVocab::element_add) {
				is_front = true;
				break;
			}
			rsa = rsa->parent();
		}
		s->add(c,is_front);
	}
}


template<typename Element>
Element *
flow_element_factory(AttributeMap &,Reader &)
{
	return NULL; // failure
}

// specialize some flow_element_factory implementations:

template<>
flow::Condition *
flow_element_factory<flow::Condition>(AttributeMap & amap, Reader & reader)
{
	const char * condfunction_str =
		amap.getOrThrow(XMLVocab::attr_name).c_str();
	bool is_negated = amap.getOrThrow(XMLVocab::attr_isnegated).boolVal();
	int argno = amap.getOrThrow(XMLVocab::attr_argno).intVal();
	int unionnumber = amap.getOrThrow(XMLVocab::attr_unionnumber).intVal();
	ConditionFn condfn = reader.fromThisScope()->lookup_conditional(
		  condfunction_str
		, argno
		, unionnumber );
	if(!condfn) {
		std::string ex_msg = "Could not create condition function ";
		ex_msg += condfunction_str;
		throw ReaderException(ex_msg.c_str());
	}
	return  ( reader.allow_addition()
		? new flow::Condition(condfn,is_negated)
		: NULL);
}

template<>
flow::Guard *
flow_element_factory<flow::Guard>(AttributeMap & amap, Reader & reader)
{
	const char * name =
		amap.getOrThrow(XMLVocab::attr_name).c_str();
	AtomicMapAbstract * atomicmap = reader.fromThisScope()->lookup_atomic_map(name);
	return new flow::Guard(atomicmap,name);
}

template<>
flow::GuardReference *
flow_element_factory<flow::GuardReference>(AttributeMap & amap, Reader & reader)
{
	bool is_late = amap.getOrThrow(XMLVocab::attr_late).boolVal();
	int wtype = amap.getOrThrow(XMLVocab::attr_wtype).intVal();
	int unionnumber = amap.getOrThrow(XMLVocab::attr_unionnumber).intVal();
	const char * hash = amap.getOrThrow(XMLVocab::attr_hash).c_str();
	const char * guard_name = amap.getOrThrow(XMLVocab::attr_name).c_str();
	flow::Guard * g = reader.find<flow::Guard>(guard_name);
	flow::GuardReference * result = NULL;
	if(reader.allow_addition()) {
		result = new flow::GuardReference(g,wtype,is_late);
		GuardTransFn guardfn =
                        reader.fromThisScope()->lookup_guard_translator(
                                  guard_name
                                , unionnumber
                                , hash
                                , wtype
                                , is_late);
		if(!guardfn) {
			std::string ex_msg = "guard translator function not found for ";
			ex_msg += guard_name;
			throw ReaderException(ex_msg.c_str());
		}
		result->setGuardFn(guardfn);
	}
	return result;
}

template<>
flow::Case *
flow_element_factory<flow::Case>(AttributeMap & amap, Reader & reader)
{
	const char * target_name =
		amap.getOrThrow(XMLVocab::attr_nodetarget).c_str();
	flow::Case * result = NULL;
	flow::Node * from_node = reader.get<flow::Node>();
	int outputunionnumber = from_node->outputUnionNumber();
	if(reader.allow_addition()) {
		result = new flow::Case(target_name);
		oflux_log_debug("flow_element_factory<flow::Case> %p %s adding t %s\n"
			, result
			, from_node->getName()
			, target_name);
		reader.addTarget(
			  result
			, target_name
			, outputunionnumber); // + scope_name
	}
	return result;
}

template<>
flow::Successor *
flow_element_factory<flow::Successor>(AttributeMap & amap, Reader & reader)
{
	const char * name = amap.getOrThrow(XMLVocab::attr_name).c_str(); // throws if failure
	flow::Successor * result = NULL;
	if(!reader.allow_addition()) {
		result = reader.get<flow::SuccessorList>()->get_successor(name);
			// NULL on failure
	} else {
		result = new flow::Successor(name);
	}
	return result;
}


template<>
flow::SuccessorList *
flow_element_factory<flow::SuccessorList>(AttributeMap &, Reader & reader)
{
	flow::SuccessorList * result = NULL;
	if(!reader.allow_addition()) {
		flow::Node * n = reader.get<flow::Node>();
		result = n ? n->successor_list() : NULL;
	} else {
		result = new flow::SuccessorList();
	}
	return result;
}

template<>
flow::Node *
flow_element_factory<flow::Node>(AttributeMap & amap, Reader & reader)
{
	CreateNodeFn createfn =
		reader.fromThisScope()->lookup_node_function(amap.getOrThrow(XMLVocab::attr_function).c_str());
	bool is_external = amap.getOrThrow(XMLVocab::attr_external).boolVal();
	flow::Node * result = NULL;
	const char * node_name = amap.getOrThrow(XMLVocab::attr_name).c_str();

	if(is_external) {
		result = reader.find<flow::Node>(node_name);
		reader.allow_addition(false);
	} else if(reader.allow_addition()) {
		result = new flow::Node(
			  node_name
			, createfn
			, amap.getOrThrow(XMLVocab::attr_iserrhandler).boolVal()
			, amap.getOrThrow(XMLVocab::attr_source).boolVal()
			, amap.getOrThrow(XMLVocab::attr_detached).boolVal()
			, amap.getOrThrow(XMLVocab::attr_inputunionnumber).intVal()
			, amap.getOrThrow(XMLVocab::attr_outputunionnumber).intVal());
	}
	return result;
}

template<>
flow::Library *
flow_element_factory<flow::Library>(AttributeMap &amap, Reader & reader)
{
	flow::Library * lib =
		new flow::Library(
			  reader.pluginLibDir()
			, amap.getOrThrow(XMLVocab::attr_name).c_str());
	flow::Library * prev_lib =
		reader.get<flow::Flow>()->getPrevLibrary(lib->getName().c_str());
	lib->addDependency("");
	if(prev_lib) {
		delete lib;
		lib = prev_lib;
	}
	return lib;
}

template<>
flow::Flow *
flow_element_factory<flow::Flow>(AttributeMap &, Reader & reader)
{
	const flow::Flow * ef =
		reader.existing_flow(); // for SIGHUP
	flow::Flow * f = reader.get<flow::Flow>(); // for plugins
	if(reader.allow_addition() && !f) {
		f = ( ef ? new flow::Flow(*ef) : new flow::Flow());
	}
	return f;
}

template<typename T>
const char * ElementName<T>::name = "unknown";

template<> const char * ElementName<flow::Flow>::name = XMLVocab::element_flow;
template<> const char * ElementName<flow::Guard>::name = XMLVocab::element_guard;
template<> const char * ElementName<flow::GuardReference>::name = XMLVocab::element_guardref;
template<> const char * ElementName<flow::Node>::name = XMLVocab::element_node;
template<> const char * ElementName<flow::SuccessorList>::name = XMLVocab::element_successorlist;
template<> const char * ElementName<flow::Successor>::name = XMLVocab::element_successor;
template<> const char * ElementName<flow::Case>::name = XMLVocab::element_case;

const char * Reader::currentDir = ".";

Reader::Reader(
	  flow::FunctionMaps *fmaps
	, const char * pluginxmldir
	, const char * pluginlibdir
	, void * initpluginparams
	, const flow::Flow * existing_flow)
	: _plugin_lib_dir(pluginlibdir ? pluginlibdir : Reader::currentDir)
	, _plugin_xml_dir(pluginxmldir ? pluginxmldir : Reader::currentDir)
	, _init_plugin_params(initpluginparams)
	, _existing_flow(existing_flow)
	, _reader_state(NULL)
	, _allow_addition(true)
{
	std::vector<std::string> emptyvec;
	_scoped_fmaps.add("",fmaps,emptyvec);
}

flow::Flow *
Reader::read( const char * filename )
{
	flow::FlowHolder *  holder = new flow::FlowHolder();
	ReaderState<flow::FlowHolder> initial_reader_state("",holder,NULL,*this);
	_reader_state = &initial_reader_state;
        readxmlfile(filename);
        _depends_visited.addDependency(filename);
        readxmldir(holder);
	flow::Flow * flow = holder->flow();

	// finalize the flow:
        for(int i = 0; i < (int)_add_targets.size(); i++) {
                _add_targets[i].execute(flow);
        }
        for(int i = 0; i < (int)_set_error_handlers.size(); i++) {
                _set_error_handlers[i].execute(flow);
        }
        flow->pretty_print(); // to the log
	return flow;
}

void
Reader::readxmlfile( const char * filename )
{
        std::ifstream in(filename);

        if ( !in ) {
                throw ReaderException("Cannot open XML config file.");
        }

        XML_Parser p = XML_ParserCreate(NULL);
        if ( !p ) {
                throw ReaderException("Cannot create the XML parser!");
        }
        XML_SetUserData(p, this);
        XML_SetElementHandler(p, Reader::startHandler, Reader::endHandler);
        XML_SetCharacterDataHandler(p, Reader::dataHandler);
        XML_SetCommentHandler(p, Reader::commentHandler);

        int done,len;
        char buff[XML_READER_MAX_LINE +1];

        while ( in.getline(buff, XML_READER_MAX_LINE) ) {
                len = strlen(buff);
                done = in.eof();
                if ( XML_Parse(p, buff, len, done) == XML_STATUS_ERROR ) {
                        throw ReaderException("Error in parsing XML file");
                }
        }
        in.close();
        XML_ParserFree(p);
}

void Reader::readxmldir(flow::FlowHolder * flow_holder)
{
        const char * pluginxmldir = _plugin_xml_dir;
        DIR * dir = ::opendir(pluginxmldir);
        if(!dir) {
                oflux_log_warn("xml::Reader::readxmldir() directory %s does not exist or cannot be opened\n",pluginxmldir);
                return;
        }
        struct dirent * dir_entry;
	AttributeMap empty_map;
	//pushState("flow",empty_map);
	_reader_state = new ReaderState<flow::Flow>(
		  XMLVocab::element_flow
		, flow_holder->flow()
		, getState()
		, *this);
        while((dir_entry = ::readdir(dir)) != NULL) {
                std::string filename = dir_entry->d_name;
                size_t found = filename.find_last_of(".");
                if(filename.substr(found+1) == "xml" ) {
                        filename = (std::string) pluginxmldir + "/" + filename;
                        if(!_depends_visited.isDependency(filename.c_str())) {
                                readxmlfile(filename.c_str());
                        }
                }
        }
	delete popState();
        ::closedir(dir);
}


void
Reader::pushState(
	  const char * element_str
	, AttributeMap & map)
{
	struct ElementStruct {
		const char * element_name;
		ReaderStateFun factory;
	};
	static ElementStruct element_lookup[] = {
		  { XMLVocab::element_flow, ReaderState<flow::Flow>::factory }
		, { XMLVocab::element_plugin, ReaderStateAddition::factory }
		, { XMLVocab::element_library, ReaderState<flow::Library>::factory }
		, { XMLVocab::element_depend, ReaderStateDepend::factory }
		, { XMLVocab::element_guard, ReaderState<flow::Guard>::factory }
		, { XMLVocab::element_guardprecedence, ReaderStateGuardPrecedence::factory }
		, { XMLVocab::element_guardref, ReaderState<flow::GuardReference>::factory }
		//, { XMLVocab::element_argument, ReaderState<flow::Argument>::factory }
		, { XMLVocab::element_add, ReaderStateAddition::factory }
		, { XMLVocab::element_remove, ReaderStateRemoval::factory }
		, { XMLVocab::element_node, ReaderState<flow::Node>::factory }
		, { XMLVocab::element_successorlist, ReaderState<flow::SuccessorList>::factory }
		, { XMLVocab::element_successor, ReaderState<flow::Successor>::factory }
		, { XMLVocab::element_errorhandler, ReaderStateErrorHandler::factory }
		, { XMLVocab::element_case, ReaderState<flow::Case>::factory }
		, { XMLVocab::element_condition, ReaderState<flow::Condition>::factory }
		, { NULL, NULL }
	};
	ElementStruct * esptr = &element_lookup[0];
	while(esptr->element_name != NULL
			&& 0 != strcmp(esptr->element_name,element_str)) {
		++esptr;
	}
	if(esptr->element_name == NULL) {
		std::string ex_msg = "Unknown element ";
		ex_msg += element_str;
		throw ReaderException(ex_msg.c_str());
	}
	const char * elname = esptr->element_name;
	if(elname == XMLVocab::element_plugin) {
		elname = XMLVocab::element_flow;
	}
	ReaderStateAbstract * rsa = (*(esptr->factory))(
		  elname
		, map
		, *this);
	_reader_state = rsa;
}

ReaderStateAbstract *
Reader::popState()
{
	ReaderStateAbstract * rsa = _reader_state;
	_reader_state = rsa->parent();
	if(rsa->elementName() == XMLVocab::element_node) {
		allow_addition(true); // reset this state in case the node was external
	}
	return rsa;
}


void
Reader::startHandler(void *data, const char *el, const char **attr)
{
	Reader * reader = reinterpret_cast<Reader *>(data);
	AttributeMap map;
	fillAttributeMap(map,attr);
	reader->pushState(el,map);
}

void
Reader::endHandler(void *data, const char *el)
{
	Reader * reader = reinterpret_cast<Reader *>(data);
	delete reader->popState();
}

void Reader::dataHandler(void *data, const char *xml_data, int len)
{
        // not used
}

void Reader::commentHandler(void *data, const char *comment)
{
        // not used
}


flow::Flow *
read(     const char * filename
	, flow::FunctionMaps *fmaps
	, const char * pluginxmldir
	, const char * pluginlibdir
	, void * initpluginparams
	, const flow::Flow * existing_flow)
{
        Reader reader(    fmaps
			, pluginxmldir
			, pluginlibdir
			, initpluginparams
			, existing_flow);
	return reader.read(filename);
}

} // namespace xml
} // namespace oflux
