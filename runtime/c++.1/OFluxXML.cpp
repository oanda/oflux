#include <string.h>
#include "OFluxXML.h"
#include "OFluxFlow.h"
#include "OFluxLibrary.h"
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


/**
 * @class Reader
 * @brief reads an XML flux file given the mappings needed to compile a flow
 * The mappings are used to build a flow with links to the event and
 * conditional function mappings needed to run the flow in the run time.
 */
class Reader {
public:
        Reader(   const char * filename
		, flow::FunctionMaps *fmaps
		, const char * pluginxmldir
		, const char * pluginlibdir
		, void * initpluginparams
		, const flow::Flow * existing_flow);

        /**
         * @brief read a file
         *
         * @param filename  is the file that is being read
         *
         **/
        void read(const char * filename);

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
        flow::Flow * flow() { return _flow; }
        flow::Case * flow_case() { return _flow_case; }
        void new_flow_case(const char * targetnodename, int node_output_unionnumber)
        {
                _flow_case = new flow::Case(NULL);
                AddTarget at(_flow_case,targetnodename, node_output_unionnumber, this, _scope_name);
                _add_targets.push_back(at);
        }
        void addremove_flow_case(bool front=false)
        {
		if(isDeletion()) {
			_flow_successor->remove(_flow_case);
		} else {
			_flow_successor->add(_flow_case, front);
		}
                _flow_case = NULL;
        }
        void new_flow_successor(const char * name)
        {
                if(_is_external_node) {
                        _flow_successor = _flow_successor_list->get_successor(name);
                }
                if(_flow_successor) {
                        _is_existing_successor = true;
                } else {
                        _flow_successor = new flow::Successor(name);
                }
        }
        void addremove_flow_successor()
        {
                if(_is_existing_successor) {
                        _is_existing_successor = false;
			if(isDeletion()) {
				_flow_successor_list->remove(_flow_successor);
			}
                } else {
                        _flow_successor_list->add(_flow_successor);
                }
                _flow_successor = NULL;
        }
        void new_flow_successor_list() { }
        void add_flow_successor_list() { }
        void new_flow_guard(const char * name, AtomicMapAbstract * amap)
        { _flow->addGuard(new flow::Guard(amap,name)); }
        void new_flow_guardprecedence(const char * before, const char * after)
        { _flow->addGuardPrecedence(before,after); }
        void new_flow_guard_reference(flow::Guard * fg
                , int unionnumber
                , const char * hash
                , int wtype
		, bool late)
        {
                _flow_guard_reference = new flow::GuardReference(fg,wtype,late);
                _flow_guard_ref_unionnumber = unionnumber;
                _flow_guard_ref_hash = hash;
                _flow_guard_ref_wtype = wtype;
        }
        void complete_flow_guard_reference()
        {
                const char * name = _flow_guard_reference->getName().c_str();
                GuardTransFn guardfn =
			_scoped_fmaps.get(_scope_name.c_str())->lookup_guard_translator(
				  name
				, _flow_guard_ref_unionnumber
				, _flow_guard_ref_hash.c_str()
				, _flow_guard_ref_wtype
				, _flow_guard_reference->late());
                assert(guardfn != NULL);
                _flow_guard_reference->setGuardFn(guardfn);
                _flow_node->addGuard(_flow_guard_reference);
                _flow_guard_reference = NULL;
        }
        //void flow_guard_ref_add_argument(int an) { _flow_guard_ref_args.push_back(an); }
        flow::Node * flow_node() { return _flow_node; }
        void new_flow_node(const char * name, CreateNodeFn createfn,
                        bool is_error_handler,
                        bool is_src,
                        bool is_detached,
                        int input_unionnumber,
                        int output_unionnumber)
        {
                _flow_node = new flow::Node(
                                        name,
                                        createfn,
                                        is_error_handler,
                                        is_src,
                                        is_detached,
                                        input_unionnumber,
                                        output_unionnumber);
                _flow_successor_list = &(_flow_node->successor_list());
        }
        void add_flow_node()
        {
                if(_is_external_node) {
                        _is_external_node = false;
                } else {
                        _flow->add(_flow_node);
                }
                _flow_node = NULL;
                _flow_successor_list = NULL;
        }
        bool find_flow_node(const char * name)
        {
                _flow_node = _flow->get(name);
                if(_flow_node) {
                        _flow_successor_list = &(_flow_node->successor_list());
                        return true;
                }
                return false;
        }
        void new_library(const char * filename);
        void add_library();
        void new_depend(const char * depend);
        void setErrorHandler(const char * err_node_name)
        {
                SetErrorHandler seh(_flow_node,err_node_name);
                _set_error_handlers.push_back(seh);
        }
        bool isExternalNode() { return _is_external_node; }
        void setIsExternalNode(bool is_external_node)
	{ _is_external_node = is_external_node; }
        bool isAddition() { return _is_add; }
        void setAddition(bool add) { _is_add = add; }
        bool isDeletion() { return _is_remove; }
        void setDeletion(bool remove) { _is_remove = remove; }

	typedef boost::shared_ptr<ScopedFunctionMaps::Scope> ScopePtr;

	ScopePtr fromThisScope(const char * s = NULL)
	{ return _scoped_fmaps.get(s ? s : _scope_name.c_str()); }

protected:
        /**
         * @brief just connect the forward flow node references stored up
         */
        void readxmldir();
        void readxmlfile(
		  const char * filename
		, XML_StartElementHandler startHandler
		, XML_EndElementHandler endHandler);

        void finalize();

private:
	std::string                       _scope_name;
        ScopedFunctionMaps                _scoped_fmaps;
        flow::Flow *                      _flow;
        flow::Node *                      _flow_node;
        flow::SuccessorList *             _flow_successor_list;
        flow::Successor *                 _flow_successor;
        flow::Case *                      _flow_case;
        flow::GuardReference *            _flow_guard_reference;
        int                               _flow_guard_ref_unionnumber;
        std::string                       _flow_guard_ref_hash;
        int                               _flow_guard_ref_wtype;
        std::vector<AddTarget>            _add_targets;
        std::vector<SetErrorHandler>      _set_error_handlers;
        bool                              _is_external_node;
        bool                              _is_existing_successor;
        bool                              _is_add;
        bool                              _is_remove;
        flow::Library *                   _library;
        const char *                      _plugin_lib_dir;
        const char *                      _plugin_xml_dir;
        DependencyTracker                 _depends_visited;
        void *                            _init_plugin_params;
};

void AddTarget::execute(flow::Flow * f)
{
        flow::Node *fsrc = f->get(_name);
        assert(fsrc);
        _fc->setTargetNode(fsrc);
        _target_input_unionnumber = fsrc->inputUnionNumber();
        FlatIOConversionFun fiocf =
		_xmlreader->fromThisScope(_scope_name.c_str())->lookup_io_conversion(_node_output_unionnumber, _target_input_unionnumber);
        if(fiocf) {
                assert(_fc->ioConverter() == &flow::IOConverter::standard_converter);
                _fc->setIOConverter(new flow::IOConverter(fiocf));
        }
}

void SetErrorHandler::execute(flow::Flow * f)
{
        flow::Node *fsrc = f->get(_name);
        assert(fsrc);
        assert(fsrc->getIsErrorHandler());
        _fn->setErrorHandler(fsrc);
}

Reader::Reader(
	  const char * filename
	, flow::FunctionMaps *fmaps
	, const char * pluginxmldir
	, const char * pluginlibdir
	, void * initpluginparams
	, const flow::Flow * existing_flow)
        : _flow(existing_flow ? new flow::Flow(*existing_flow) : new flow::Flow())
        , _flow_node(NULL)
        , _flow_successor_list(NULL)
        , _flow_successor(NULL)
        , _flow_case(NULL)
        , _flow_guard_reference(NULL)
	, _flow_guard_ref_unionnumber(0)
	, _flow_guard_ref_wtype(0)
        , _is_external_node(false)
        , _is_existing_successor(false)
        , _is_add(false)
        , _is_remove(false)
        , _library(NULL)
        , _plugin_lib_dir(pluginlibdir)
        , _plugin_xml_dir(pluginxmldir)
        , _init_plugin_params(initpluginparams)
{
	std::vector<std::string> emptyvec;
        _scoped_fmaps.add("",fmaps,emptyvec);
        read(filename);
}

void Reader::read(const char * filename)
{
        readxmlfile(filename, Reader::startMainHandler, Reader::endMainHandler);
        _depends_visited.addDependency(filename);
        if(_plugin_lib_dir == NULL) {
                _plugin_lib_dir = ".";
        }
        if(_plugin_xml_dir == NULL) {
                _plugin_xml_dir = ".";
        }
        readxmldir();
        finalize();
}

void Reader::readxmlfile(
	  const char * filename
	, XML_StartElementHandler startHandler
	, XML_EndElementHandler endHandler)
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
        XML_SetElementHandler(p, startHandler, endHandler);
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

void Reader::readxmldir()
{
        const char * pluginxmldir = _plugin_xml_dir;
        DIR * dir = ::opendir(pluginxmldir);
        if(!dir) {
                oflux_log_warn("xml::Reader::readxmldir() directory %s does not exist or cannot be opened\n",pluginxmldir);
                return;
        }
        struct dirent * dir_entry;
        while((dir_entry = ::readdir(dir)) != NULL) {
                std::string filename = dir_entry->d_name;
                size_t found = filename.find_last_of(".");
                if(filename.substr(found+1) == "xml" ) {
                        filename = (std::string) pluginxmldir + "/" + filename;
                        if(!_depends_visited.isDependency(filename.c_str())) {
                                readxmlfile(filename.c_str(), Reader::startPluginHandler, Reader::endPluginHandler);
                        }
                }
        }
        ::closedir(dir);
}

void Reader::finalize()
{
        for(int i = 0; i < (int)_add_targets.size(); i++) {
                _add_targets[i].execute(_flow);
        }
        for(int i = 0; i < (int)_set_error_handlers.size(); i++) {
                _set_error_handlers[i].execute(_flow);
        }
        _flow->pretty_print();
}

void Reader::startMainHandler(void *data, const char *el, const char **attr)
{
        Reader * pthis = static_cast<Reader *> (data);
        const char * el_name = NULL;
        const char * el_argno = NULL;
        const char * el_nodetarget = NULL;
        const char * el_source = NULL;
        const char * el_isnegated = NULL;
        const char * el_iserrhandler = NULL;
        const char * el_unionnumber = NULL;
        const char * el_inputunionnumber = NULL;
        const char * el_outputunionnumber = NULL;
        const char * el_detached = NULL;
        const char * el_wtype = NULL;
        const char * el_function = NULL;
        const char * el_hash = NULL;
        const char * el_before = NULL;
        const char * el_after = NULL;
        const char * el_late = NULL;
        for(int i = 0; attr[i]; i += 2) {
                if(strcmp(attr[i],"name") == 0) {
                        el_name = attr[i+1];
                } else if(strcmp(attr[i],"argno") == 0) {
                        el_argno = attr[i+1];
                } else if(strcmp(attr[i],"nodetarget") == 0) {
                        el_nodetarget = attr[i+1];
                } else if(strcmp(attr[i],"source") == 0) {
                        el_source = attr[i+1];
                } else if(strcmp(attr[i],"isnegated") == 0) {
                        el_isnegated = attr[i+1];
                } else if(strcmp(attr[i],"iserrhandler") == 0) {
                        el_iserrhandler = attr[i+1];
                } else if(strcmp(attr[i],"detached") == 0) {
                    	el_detached = attr[i+1];
                } else if(strcmp(attr[i],"unionnumber") == 0) {
                        el_unionnumber = attr[i+1];
                } else if(strcmp(attr[i],"inputunionnumber") == 0) {
                        el_inputunionnumber = attr[i+1];
                } else if(strcmp(attr[i],"outputunionnumber") == 0) {
                        el_outputunionnumber = attr[i+1];
                } else if(strcmp(attr[i],"after") == 0) {
                        el_after = attr[i+1];
                } else if(strcmp(attr[i],"before") == 0) {
                        el_before = attr[i+1];
                } else if(strcmp(attr[i],"late") == 0) {
                        el_late = attr[i+1];
                } else if(strcmp(attr[i],"hash") == 0) {
                        el_hash = attr[i+1];
                } else if(strcmp(attr[i],"wtype") == 0) {
                        el_wtype = attr[i+1];
                } else if(strcmp(attr[i],"function") == 0) {
                        el_function = attr[i+1];
                }
        }
        bool is_late = (el_late ? strcmp(el_late,"true")==0 : false);
        bool is_source = (el_source ? strcmp(el_source,"true")==0 : false);
        bool is_negated = (el_isnegated ? strcmp(el_isnegated,"true")==0 : false);
        bool is_errorhandler = (el_iserrhandler ? strcmp(el_iserrhandler,"true")==0 : false);
        bool is_detached = (el_detached ? strcmp(el_detached,"true")==0 : false);
        int argno = (el_argno ? atoi(el_argno) : 0);
        int unionnumber = (el_unionnumber ? atoi(el_unionnumber) : 0);
        int inputunionnumber = (el_inputunionnumber ? atoi(el_inputunionnumber) : 0);
        int outputunionnumber = (el_outputunionnumber ? atoi(el_outputunionnumber) : 0);
        int wtype = (el_wtype ? atoi(el_wtype) : 0);
        const char * hash = el_hash;
        if(strcmp(el,"argument") == 0) {
                // has attributes: argno
                // has no children
                //pthis->flow_guard_ref_add_argument(argno);
        } else if(strcmp(el,"guard") == 0) {
                // has attributes: name
                // has no children
                AtomicMapAbstract * amap = pthis->fromThisScope()->lookup_atomic_map(el_name);
                assert(amap);
                pthis->new_flow_guard(el_name, amap);
        } else if(strcmp(el,"guardprecedence") == 0) {
                // has attributes: before, after
                // has no children
                pthis->new_flow_guardprecedence(el_before, el_after);
        } else if(strcmp(el,"guardref") == 0) {
                // has attributes: name, unionnumber, wtype
                // has children: argument(s)
                flow::Guard * fg = pthis->flow()->getGuard(el_name);
                assert(fg);
                pthis->new_flow_guard_reference(fg, unionnumber, hash, wtype, is_late);
        } else if(strcmp(el,"condition") == 0) {
                // has attributes: name, argno, isnegated
                // has no children
                ConditionFn condfn = pthis->fromThisScope()->lookup_conditional(el_name,argno,unionnumber);
                assert(condfn != NULL);
                flow::Condition * fc = new flow::Condition(condfn,is_negated);
                pthis->flow_case()->add(fc);
        } else if(strcmp(el,"case") == 0) {
                // has attributes: nodetarget
                // has children: condition
                pthis->new_flow_case(el_nodetarget,pthis->flow_node()->outputUnionNumber());
        } else if(strcmp(el,"successor") == 0) {
                // has attribute: name
                // has children: case
                pthis->new_flow_successor(el_name);
        } else if(strcmp(el,"successorlist") == 0) {
                // has no attributes
                // has children: successor
                pthis->new_flow_successor_list();
        } else if(strcmp(el,"errorhandler") == 0) {
                // has attributes: name
                // has no children
                pthis->setErrorHandler(el_name);
        } else if(strcmp(el,"node") == 0) {
                // has attributes: name, source, inputunionnumber, outputunionnumber
                // has children: errorhandler, guardref(s), successorlist
                CreateNodeFn createfn = pthis->fromThisScope()->lookup_node_function(el_function);
                assert(createfn != NULL);
                pthis->new_flow_node(el_name, createfn, is_errorhandler, is_source, is_detached, inputunionnumber, outputunionnumber);
        }
}

void Reader::endMainHandler(void *data, const char *el)
{
        Reader * pthis = static_cast<Reader *> (data);
        if(strcmp(el,"argument") == 0) {
                // do nothing
        } else if(strcmp(el,"guard") == 0) {
                // do nothing
        } else if(strcmp(el,"guardref") == 0) {
                pthis->complete_flow_guard_reference();
        } else if(strcmp(el,"condition") == 0) {
                // do nothing
        } else if(strcmp(el,"node") == 0) {
                pthis->add_flow_node();
        } else if(strcmp(el,"case") == 0) {
                pthis->addremove_flow_case();
        } else if(strcmp(el,"successor") == 0) {
                pthis->addremove_flow_successor();
        } else if(strcmp(el,"successorlist") == 0) {
                pthis->add_flow_successor_list();
        } else if(strcmp(el,"errorhandler") == 0) {
                // do nothing
        }
}

void Reader::startPluginHandler(void *data, const char *el, const char **attr)
{
        Reader * pthis = static_cast<Reader *> (data);

        // node attributes
        const char * el_name = NULL;
        const char * el_function = NULL;
        const char * el_source = NULL;
        const char * el_iserrhandler = NULL;
        const char * el_detached = NULL;
        const char * el_external = NULL;
        const char * el_inputunionnumber = NULL;
        const char * el_outputunionnumber = NULL;

        // condition attributes
        const char * el_argno = NULL;
        const char * el_isnegated = NULL;
        const char * el_unionnumber = NULL;

        // case attribute
        const char * el_nodetarget = NULL;

        // guard attributes
        const char * el_late = NULL;
        const char * el_before = NULL;
        const char * el_after = NULL;
        const char * el_wtype = NULL;
        const char * el_hash = NULL;

        for(int i = 0; attr[i]; i += 2) {
                // node attributes
                if(strcmp(attr[i],"name") == 0) {
                    	el_name = attr[i+1];
                } else if(strcmp(attr[i],"function") == 0) {
                    	el_function = attr[i+1];
                } else if(strcmp(attr[i],"source") == 0) {
                    	el_source = attr[i+1];
                } else if(strcmp(attr[i],"iserrhandler") == 0) {
                    	el_iserrhandler = attr[i+1];
                } else if(strcmp(attr[i],"detached") == 0) {
                    	el_detached = attr[i+1];
                } else if(strcmp(attr[i],"external") == 0) {
                    	el_external = attr[i+1];
                } else if(strcmp(attr[i],"inputunionnumber") == 0) {
                    	el_inputunionnumber = attr[i+1];
                } else if(strcmp(attr[i],"outputunionnumber") == 0) {
                    	el_outputunionnumber = attr[i+1];
                // condition attributes
                } else if(strcmp(attr[i],"argno") == 0) {
                    	el_argno = attr[i+1];
                } else if(strcmp(attr[i],"isnegated") == 0) {
                    	el_isnegated = attr[i+1];
                } else if(strcmp(attr[i],"unionnumber") == 0) {
                    	el_unionnumber = attr[i+1];
                // case attribute
                } else if(strcmp(attr[i],"nodetarget") == 0) {
                    	el_nodetarget = attr[i+1];
                // guard attributes
                } else if(strcmp(attr[i],"after") == 0) {
                    	el_after = attr[i+1];
                } else if(strcmp(attr[i],"before") == 0) {
                    	el_before = attr[i+1];
                } else if(strcmp(attr[i],"late") == 0) {
                    	el_late = attr[i+1];
                } else if(strcmp(attr[i],"hash") == 0) {
                    	el_hash = attr[i+1];
                } else if(strcmp(attr[i],"wtype") == 0) {
                    	el_wtype = attr[i+1];
                }
        }

        bool is_late = (el_late ? strcmp(el_late,"true")==0 : false);
        bool is_source = (el_source ? strcmp(el_source,"true")==0 : false);
        bool is_errorhandler = (el_iserrhandler ? strcmp(el_iserrhandler,"true")==0 : false);
        bool is_detached = (el_detached ? strcmp(el_detached,"true")==0 : false);
        bool is_external = (el_external ? strcmp(el_external,"true")==0 : false);
        bool is_negated = (el_isnegated ? strcmp(el_isnegated,"true")==0 : false);

        int argno = (el_argno ? atoi(el_argno) : 0);
        int unionnumber = (el_unionnumber ? atoi(el_unionnumber) : 0);
        int inputunionnumber = (el_inputunionnumber ? atoi(el_inputunionnumber) : 0);
        int outputunionnumber = (el_outputunionnumber ? atoi(el_outputunionnumber) : 0);
        int wtype = (el_wtype ? atoi(el_wtype) : 0);
        const char * hash = el_hash;

        bool is_ok_to_create = pthis->isAddition() || (!pthis->isExternalNode());

        if(strcmp(el,"plugin") == 0) {
                // has attribute:
                // has children: node, guard, library
        } else if(strcmp(el,"library") == 0) {
                // has attribute: name
                // has children: depend
                pthis->new_library(el_name);
        } else if(strcmp(el,"depend") == 0) {
                // has attributes: name
                // has no children
                pthis->new_depend(el_name);
        } else if(strcmp(el,"guard") == 0) {
                // has attributes: name
                // has no children
                AtomicMapAbstract * amap = pthis->fromThisScope()->lookup_atomic_map(el_name);
                assert(amap);
                pthis->new_flow_guard(el_name, amap);
        } else if(strcmp(el,"guardprecedence") == 0) {
                // has attributes: before, after
                // has no children
                pthis->new_flow_guardprecedence(el_before, el_after);
        } else if(strcmp(el,"guardref") == 0 && is_ok_to_create) {
                // has attributes: name, unionnumber, wtype
                // has children: argument(s)
                flow::Guard * fg = pthis->flow()->getGuard(el_name);
                assert(fg);
                pthis->new_flow_guard_reference(fg, unionnumber, hash, wtype, is_late);
        } else if(strcmp(el,"argument") == 0 && is_ok_to_create) {
                // has attributes: argno
                // had no children
                //pthis->flow_guard_ref_add_argument(argno);
        } else if(strcmp(el,"add") == 0) {
                pthis->setAddition(true);
        } else if(strcmp(el,"remove") == 0) {
                pthis->setDeletion(true);
        } else if(strcmp(el,"node") == 0) {
                // has attributes: name, function, source, iserrhandler, detached, external
                // has children: errorhandler, guardref(s), successorlist
                if(is_external) {
                        pthis->setIsExternalNode(true);
                        bool foundNode = pthis->find_flow_node(el_name);
                        assert(foundNode);
                } else {
                        CreateNodeFn createfn = pthis->fromThisScope()->lookup_node_function(el_function);
                        assert(createfn != NULL);
                        pthis->new_flow_node(el_name, createfn, is_errorhandler, is_source, is_detached, inputunionnumber, outputunionnumber);
                }
        } else if(strcmp(el,"successorlist") == 0) {
                // has no attribute
                // has children: successor
                // do nothing
        } else if(strcmp(el,"successor") == 0) {
                // has attribute: name
                // has children: case
                pthis->new_flow_successor(el_name); // should find if external
        } else if(strcmp(el,"errorhandler") == 0) {
                // has attributes: name
                // has no children
                pthis->setErrorHandler(el_name);
        } else if(strcmp(el,"case") == 0 && is_ok_to_create) {
                // has attributes: nodetarget
                // has children: condition
                // no virtual nodetarget allowed in a plugin
                pthis->new_flow_case(el_nodetarget, pthis->flow_node()->outputUnionNumber());
        } else if(strcmp(el,"condition") == 0 && is_ok_to_create) {
                // has attributes: name, argno, isnegated
                // has no children
                ConditionFn condfn = pthis->fromThisScope()->lookup_conditional(el_name,argno,unionnumber);
                assert(condfn != NULL);
                flow::Condition * fc = new flow::Condition(condfn,is_negated);
                pthis->flow_case()->add(fc);
        }
}

void Reader::endPluginHandler(void *data, const char *el)
{
        Reader * pthis = static_cast<Reader *> (data);
        bool is_ok_to_create = pthis->isAddition() || (!pthis->isExternalNode());
        if(strcmp(el,"plugin") == 0) {
                // do nothing
        } else if(strcmp(el,"library") == 0) {
                pthis->add_library();
        } else if(strcmp(el,"guard") == 0) {
                // do nothing
        } else if(strcmp(el,"guardref") == 0 && is_ok_to_create) {
                pthis->complete_flow_guard_reference();
        } else if(strcmp(el,"argument") == 0 && is_ok_to_create) {
                // do nothing
        } else if(strcmp(el,"node") == 0) {
                pthis->add_flow_node();
        } else if(strcmp(el,"successorlist") == 0) {
                // do nothing essentially
                pthis->add_flow_successor_list();
        } else if(strcmp(el,"successor") == 0) {
                pthis->addremove_flow_successor();
        } else if(strcmp(el,"case") == 0 && is_ok_to_create) {
                pthis->addremove_flow_case(pthis->isAddition());
        } else if(strcmp(el,"condition") == 0 && is_ok_to_create) {
                // do nothing
        } else if(strcmp(el,"add") == 0) {
                pthis->setAddition(false);
        } else if(strcmp(el,"remove") == 0) {
                pthis->setDeletion(false);
        } else if(strcmp(el,"errorhandler") == 0) {
                // do nothing
        }
}

void Reader::dataHandler(void *data, const char *xml_data, int len)
{
        // not used
}

void Reader::commentHandler(void *data, const char *comment)
{
        // not used
}

extern "C" {
typedef flow::FunctionMaps * FlowFunctionMapFunction ();
}

void Reader::new_depend(const char * dependname)
{
        std::string depxml = _plugin_xml_dir;
        depxml += "/";
        depxml += dependname;
        depxml += ".xml";
        flow::Library * lib = _library; // preserve on the stack
	lib->addDependency(dependname);
        assert(_flow);
        if(!_flow->haveLibrary(dependname)) {
                if(_depends_visited.isDependency(depxml.c_str())) {
                        depxml += " circular dependency -- already loading";
                        throw ReaderException(depxml.c_str());
                }
                _depends_visited.addDependency(depxml.c_str());
                _library = NULL;
                readxmlfile(depxml.c_str(), Reader::startPluginHandler, Reader::endPluginHandler);
        }
        _library = lib; // restore
}

void Reader::new_library(const char * filename)
{
        _library = new flow::Library(_plugin_lib_dir,filename);
	flow::Library * prev_lib = _flow->getPrevLibrary(_library->getName().c_str());
	if(prev_lib) {
		delete _library;
		_library = prev_lib;
	}
	_library->addDependency("");
}

void Reader::add_library()
{
        bool loaded = _library->load();
        assert(loaded);
        std::string flowfunctionmapfunction = "flowfunctionmaps__";
        _library->addSuffix(flowfunctionmapfunction);

        FlowFunctionMapFunction * ffmpfun =
                _library->getSymbol<FlowFunctionMapFunction>(flowfunctionmapfunction.c_str());
        assert(ffmpfun);
	_scope_name = _library->getName();
	_scoped_fmaps.add(
		  _library->getName().c_str()
		, (*ffmpfun)()
		, _library->getDependencies());
        assert(_flow);

	_flow->addLibrary(_library, _init_plugin_params);
}

void DependencyTracker::addDependency(const char * fl)
{
        std::string filename = fl;
        canonize(filename);
        _depends_set.insert(filename);
}

bool DependencyTracker::isDependency(const char * fl)
{
        std::string filename = fl;
        canonize(filename);
        return _depends_set.find(filename) != _depends_set.end();
}

void DependencyTracker::canonize(std::string & filename)
{
        // just get rid of leading "./"
        if(filename.length() >= 2 && filename[0] == '.' && filename[1] == '/') {
                filename.erase(filename.begin());
                filename.erase(filename.begin());
        }
}


flow::Flow *
read(     const char * filename
	, flow::FunctionMaps *fmaps
	, const char * pluginxmldir
	, const char * pluginlibdir
	, void * initpluginparams
	, const flow::Flow * existing_flow)
{
        Reader reader(    filename
			, fmaps
			, pluginxmldir
			, pluginlibdir
			, initpluginparams
			, existing_flow);
        return reader.flow();
}

} // namespace xml
} // namespace oflux
