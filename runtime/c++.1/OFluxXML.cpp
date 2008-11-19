#include "OFluxXML.h"
#include "OFluxLibrary.h"
#include "OFluxLogging.h"
#include <fstream>
#include <cassert>
#include <dirent.h>

namespace oflux {

#define XML_READER_MAX_LINE 300

void AddTarget::execute(Flow * f)
{
        FlowNode *fsrc = f->get(_name);
        assert(fsrc);
        _fc->setTargetNode(fsrc);
        _target_input_unionnumber = fsrc->inputUnionNumber();
        FlatIOConversionFun fiocf = _xmlreader->lookup_io_conversion(_node_output_unionnumber, _target_input_unionnumber);
        if(fiocf) {
                assert(_fc->ioConverter() == &FlowIOConverter::standard_converter);
                _fc->setIOConverter(new FlowIOConverter(fiocf));
        }
}

void SetErrorHandler::execute(Flow * f)
{
        FlowNode *fsrc = f->get(_name);
        assert(fsrc);
        assert(fsrc->getIsErrorHandler());
        _fn->setErrorHandler(fsrc);
}

XMLReader::XMLReader(const char * filename, FlowFunctionMaps *fmaps, const char * pluginxmldir, const char * pluginlibdir)
        : _flow(new Flow())
        , _flow_node(NULL)
        , _flow_successor_list(NULL)
        , _flow_successor(NULL)
        , _flow_case(NULL)
        , _flow_guard_reference(NULL)
        , _is_external_node(false)
        , _is_existing_successor(false)
        , _is_add(false)
        , _library(NULL)
        , _plugin_lib_dir(pluginlibdir)
        , _plugin_xml_dir(pluginxmldir)
{ 
        _fmaps_vec.push_back(fmaps);
        read(filename);
}

void XMLReader::read(const char * filename)
{
        readxmlfile(filename, XMLReader::startMainHandler, XMLReader::endMainHandler);
        if(_plugin_lib_dir == NULL) {
                _plugin_lib_dir = ".";
        }
        if(_plugin_xml_dir == NULL) {
                _plugin_xml_dir = ".";
        }
        readxmldir();
        finalize();
}

void XMLReader::readxmlfile(const char * filename, XML_StartElementHandler startHandler, XML_EndElementHandler endHandler)
{
        std::ifstream in(filename);

        if ( !in ) {
                throw XMLReaderException("Cannot open XML config file.");
        }

        XML_Parser p = XML_ParserCreate(NULL);
        if ( !p ) {
                throw XMLReaderException("Cannot create the XML parser!");
        }
        XML_SetUserData(p, this);
        XML_SetElementHandler(p, startHandler, endHandler);
        XML_SetCharacterDataHandler(p, XMLReader::dataHandler);
        XML_SetCommentHandler(p, XMLReader::commentHandler);

        int done,len;
        char buff[XML_READER_MAX_LINE +1];

        while ( in.getline(buff, XML_READER_MAX_LINE) ) {
                len = strlen(buff);
                done = in.eof();
                if ( XML_Parse(p, buff, len, done) == XML_STATUS_ERROR ) {
                        throw XMLReaderException("Error in parsing XML file");
                }
        }
        in.close();
        XML_ParserFree(p);
}

void XMLReader::readxmldir()
{
        const char * pluginxmldir = _plugin_xml_dir;
        DIR * dir = ::opendir(pluginxmldir);
        assert(dir);
        struct dirent * dir_entry;
        while((dir_entry = ::readdir(dir)) != NULL) {
                std::string filename = dir_entry->d_name;
                size_t found = filename.find_last_of(".");
                if(filename.substr(found+1) == "xml" ) {
                        filename = (std::string) pluginxmldir + "/" + filename;
                        bool alreadydone = false;
                        for(int i = 0; i < (int)_depends_visited.size(); i++) {
                                if(_depends_visited[i] == filename) {
                                        alreadydone = true;
                                        break;
                                }
                        }
                        if(!alreadydone) {
                                readxmlfile(filename.c_str(), XMLReader::startPluginHandler, XMLReader::endPluginHandler);
                        }
                }
        }
        ::closedir(dir);
}

void XMLReader::finalize()
{
        for(int i = 0; i < (int)_add_targets.size(); i++) {
                _add_targets[i].execute(_flow);
        }
        for(int i = 0; i < (int)_set_error_handlers.size(); i++) {
                _set_error_handlers[i].execute(_flow);
        }
        _flow->pretty_print();
}

CreateNodeFn XMLReader::lookup_node_function(const char *n)
{
        CreateNodeFn res = NULL;
        for(int i = _fmaps_vec.size() -1; i >= 0 && res == NULL ; i--) {
                res = _fmaps_vec[i]->lookup_node_function(n);
        }
        return res;
}

ConditionFn XMLReader::lookup_conditional(const char * n, int argno, int unionnumber)
{
        ConditionFn res = NULL;
        for(int i = _fmaps_vec.size() -1; i >= 0 && res == NULL ; i--) {
                res = _fmaps_vec[i]->lookup_conditional(n,argno,unionnumber);
        }
        return res;
}

GuardTransFn XMLReader::lookup_guard_translator(const char * guardname, int union_number, long hash, int wtype)
{
        GuardTransFn res = NULL;
        for(int i = _fmaps_vec.size() -1; i >= 0 && res == NULL ; i--) {
                res = _fmaps_vec[i]->lookup_guard_translator(guardname,union_number,hash,wtype);
        }
        return res;
}

AtomicMapAbstract * XMLReader::lookup_atomic_map(const char * guardname)
{
        AtomicMapAbstract * res = NULL;
        for(int i = _fmaps_vec.size() -1; i >= 0 && res == NULL ; i--) {
                res = _fmaps_vec[i]->lookup_atomic_map(guardname);
        }
        return res;
}

FlatIOConversionFun XMLReader::lookup_io_conversion(int from_unionnumber, int to_unionnumber)
{
        FlatIOConversionFun res = NULL;
        for(int i = _fmaps_vec.size() -1; i >= 0 && res == NULL ; i--) {
                res = _fmaps_vec[i]->lookup_io_conversion(from_unionnumber,to_unionnumber);
        }
        return res;
}


void XMLReader::startMainHandler(void *data, const char *el, const char **attr)
{
        XMLReader * pthis = static_cast<XMLReader *> (data);
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
        const char * el_magicnumber = NULL;
        const char * el_wtype = NULL;
        const char * el_function = NULL;
        const char * el_hash = NULL;
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
                } else if(strcmp(attr[i],"magicnumber") == 0) {
                        el_magicnumber = attr[i+1];
                } else if(strcmp(attr[i],"hash") == 0) {
                        el_hash = attr[i+1];
                } else if(strcmp(attr[i],"wtype") == 0) {
                        el_wtype = attr[i+1];
                } else if(strcmp(attr[i],"function") == 0) {
                        el_function = attr[i+1];
                }
        }
        bool is_source = (el_source ? strcmp(el_source,"true")==0 : false);
        bool is_negated = (el_isnegated ? strcmp(el_isnegated,"true")==0 : false);
        bool is_errorhandler = (el_iserrhandler ? strcmp(el_iserrhandler,"true")==0 : false);
        bool is_detached = (el_detached ? strcmp(el_detached,"true")==0 : false);
        int argno = (el_argno ? atoi(el_argno) : 0);
        int unionnumber = (el_unionnumber ? atoi(el_unionnumber) : 0);
        int inputunionnumber = (el_inputunionnumber ? atoi(el_inputunionnumber) : 0);
        int outputunionnumber = (el_outputunionnumber ? atoi(el_outputunionnumber) : 0);
        int magicnumber = (el_magicnumber ? atoi(el_magicnumber) : 0);
        int wtype = (el_wtype ? atoi(el_wtype) : 0);
        long hash = (el_hash ? atol(el_hash) : 0);
        if(strcmp(el,"argument") == 0) {
                // has attributes: argno
                // has no children
                //pthis->flow_guard_ref_add_argument(argno);
        } else if(strcmp(el,"guard") == 0) {
                // has attributes: name, magic_number
                // has no children 
                AtomicMapAbstract * amap = pthis->lookup_atomic_map(el_name);
                assert(amap);
                pthis->new_flow_guard(el_name, magicnumber, amap);
        } else if(strcmp(el,"guardref") == 0) {
                // has attributes: name, unionnumber, wtype
                // has children: argument(s)
                FlowGuard * fg = pthis->flow()->getGuard(el_name);
                assert(fg);
                pthis->new_flow_guard_reference(fg, unionnumber, hash, wtype);
        } else if(strcmp(el,"condition") == 0) {
                // has attributes: name, argno, isnegated
                // has no children
                ConditionFn condfn = pthis->lookup_conditional(el_name,argno,unionnumber);
                assert(condfn != NULL);
                FlowCondition * fc = new FlowCondition(condfn,is_negated);
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
                CreateNodeFn createfn = pthis->lookup_node_function(el_function);
                assert(createfn != NULL);
                pthis->new_flow_node(el_name, createfn, is_errorhandler, is_source, is_detached, inputunionnumber, outputunionnumber);
        }
}

void XMLReader::endMainHandler(void *data, const char *el)
{
        XMLReader * pthis = static_cast<XMLReader *> (data);
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
                pthis->add_flow_case();
        } else if(strcmp(el,"successor") == 0) {
                pthis->add_flow_successor();
        } else if(strcmp(el,"successorlist") == 0) {
                pthis->add_flow_successor_list();
        } else if(strcmp(el,"errorhandler") == 0) {
                // do nothing
        }
}

void XMLReader::startPluginHandler(void *data, const char *el, const char **attr)
{
        XMLReader * pthis = static_cast<XMLReader *> (data);

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
        const char * el_magicnumber = NULL;
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
                } else if(strcmp(attr[i],"magicnumber") == 0) {
                    	el_magicnumber = attr[i+1];
                } else if(strcmp(attr[i],"hash") == 0) {
                    	el_hash = attr[i+1];
                } else if(strcmp(attr[i],"wtype") == 0) {
                    	el_wtype = attr[i+1];
                }
        }

        bool is_source = (el_source ? strcmp(el_source,"true")==0 : false);
        bool is_errorhandler = (el_iserrhandler ? strcmp(el_iserrhandler,"true")==0 : false);
        bool is_detached = (el_detached ? strcmp(el_detached,"true")==0 : false);
        bool is_external = (el_external ? strcmp(el_external,"true")==0 : false);
        bool is_negated = (el_isnegated ? strcmp(el_isnegated,"true")==0 : false);

        int argno = (el_argno ? atoi(el_argno) : 0);
        int unionnumber = (el_unionnumber ? atoi(el_unionnumber) : 0);
        int inputunionnumber = (el_inputunionnumber ? atoi(el_inputunionnumber) : 0);
        int outputunionnumber = (el_outputunionnumber ? atoi(el_outputunionnumber) : 0);
        int magicnumber = (el_magicnumber ? atoi(el_magicnumber) : 0);
        int wtype = (el_wtype ? atoi(el_wtype) : 0);
        long hash = (el_hash ? atol(el_hash) : 0);

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
                // has attributes: name, magicnumber
                // has no children
                AtomicMapAbstract * amap = pthis->lookup_atomic_map(el_name);
                assert(amap);
                pthis->new_flow_guard(el_name, magicnumber, amap);
        } else if(strcmp(el,"guardref") == 0 && is_ok_to_create) {
                // has attributes: name, unionnumber, wtype
                // has children: argument(s)
                FlowGuard * fg = pthis->flow()->getGuard(el_name);
                assert(fg);
                pthis->new_flow_guard_reference(fg, unionnumber, hash, wtype);
        } else if(strcmp(el,"argument") == 0 && is_ok_to_create) {
                // has attributes: argno
                // had no children
                //pthis->flow_guard_ref_add_argument(argno);
        } else if(strcmp(el,"add") == 0) { 
                pthis->setAddition(true);
        } else if(strcmp(el,"node") == 0) { 
                // has attributes: name, function, source, iserrhandler, detached, external
                // has children: errorhandler, guardref(s), successorlist 
                if(is_external) {
                        pthis->setIsExternalNode(true);
                        bool foundNode = pthis->find_flow_node(el_name);
                        assert(foundNode);
                } else {
                        CreateNodeFn createfn = pthis->lookup_node_function(el_function);
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
        } else if(strcmp(el,"case") == 0 && is_ok_to_create) {
                // has attributes: nodetarget
                // has children: condition
                // no virtual nodetarget allowed in a plugin
                pthis->new_flow_case(el_nodetarget, pthis->flow_node()->outputUnionNumber());
        } else if(strcmp(el,"condition") == 0 && is_ok_to_create) { 
                // has attributes: name, argno, isnegated
                // has no children
                ConditionFn condfn = pthis->lookup_conditional(el_name,argno,unionnumber);
                assert(condfn != NULL);
                FlowCondition * fc = new FlowCondition(condfn,is_negated);
                pthis->flow_case()->add(fc);
        }
}

void XMLReader::endPluginHandler(void *data, const char *el)
{
        XMLReader * pthis = static_cast<XMLReader *> (data);
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
                pthis->add_flow_successor();
        } else if(strcmp(el,"case") == 0 && is_ok_to_create) { 
                pthis->add_flow_case(pthis->isAddition());
        } else if(strcmp(el,"condition") == 0 && is_ok_to_create) { 
                // do nothing
        } else if(strcmp(el,"add") == 0) { 
                pthis->setAddition(false);
        }
}

void XMLReader::dataHandler(void *data, const char *xml_data, int len)
{
        // not used
}

void XMLReader::commentHandler(void *data, const char *comment)
{
        // not used
}

extern "C" {
typedef FlowFunctionMaps * FlowFunctionMapFunction ();
}

void XMLReader::new_depend(const char * dependname)
{
        std::string depxml = _plugin_xml_dir;
        depxml += "/";
        depxml += dependname;
        depxml += ".xml";
        Library * lib = _library; // preserve on the stack
        assert(_flow);
        if(!_flow->haveLibrary(dependname)) {
                for(int i = 0; i < (int)_depends_visited.size(); i++) {
                        if(depxml == _depends_visited[i]) {
                                depxml += " circular dependency -- already loading";
                                throw XMLReaderException(depxml.c_str());
                        }
                }
                _depends_visited.push_back(depxml);
                _library = NULL;
                readxmlfile(depxml.c_str(), XMLReader::startPluginHandler, XMLReader::endPluginHandler);
        }
        _library = lib; // restore
}

void XMLReader::new_library(const char * filename)
{
        _library = new Library(_plugin_lib_dir,filename);
}

void XMLReader::add_library()
{
        bool loaded = _library->load();
        assert(loaded);
        std::string flowfunctionmapfunction = "flowfunctionmaps__";
        _library->addSuffix(flowfunctionmapfunction);

        FlowFunctionMapFunction * ffmpfun = 
                _library->getSymbol<FlowFunctionMapFunction>(flowfunctionmapfunction.c_str());
        assert(ffmpfun);
        _fmaps_vec.push_back((*ffmpfun)());
        assert(_flow);
        _flow->addLibrary(_library);
}

};
