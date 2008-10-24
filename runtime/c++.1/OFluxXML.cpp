#include "OFluxXML.h"
#include "OFluxFlow.h"
#include "OFluxLibrary.h"
#include <fstream>
#include <expat.h>
#include <cassert>
#include "boost/filesystem.hpp"

namespace oflux {

namespace fs = boost::filesystem;

#define XML_READER_MAX_LINE 300

void AddTarget::execute(Flow * f, FlowFunctionMaps * fmaps)
{
	FlowNode *fsrc = f->get(_name);
	assert(fsrc);
	_fc->setTargetNode(fsrc);
        _target_input_unionnumber = fsrc->inputUnionNumber();
        FlatIOConversionFun fiocf = fmaps->lookup_io_conversion(_node_output_unionnumber,
                _target_input_unionnumber);
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

XMLReader::XMLReader(const char * filename, FlowFunctionMaps *fmaps, const char * pluginxmldir)
	: _fmaps(fmaps)
	, _plugin_fmaps(NULL)
	, _flow(new Flow())
{ 
	read(filename, pluginxmldir); 
}

void XMLReader::read(const char * filename, const char * pluginxmldir)
{
    // read plugins first, to build plugin sub-graphs for virtual nodes
    if(pluginxmldir != '\0')
    {
        readdir(pluginxmldir);
    }

    readfile(filename, XMLReader::startMainHandler, XMLReader::endMainHandler);

	finalize();
}

void XMLReader::readfile(const char * filename, XML_StartElementHandler startHandler, XML_EndElementHandler endHandler)
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

void XMLReader::readdir(const char * pluginxmldir)
{
    fs::path path( pluginxmldir );
    fs::directory_iterator itr( path );
    fs::directory_iterator end_itr; // default construction yields past-the-end
    for( ; itr != end_itr; ++itr ) {
        if( fs::is_regular( (*itr).status() ) &&
            fs::extension( (*itr).path() ) == ".xml" ) {
            readfile( (*itr).path().string().c_str(), XMLReader::startPluginHandler, XMLReader::endPluginHandler );
        }
    }
}

void XMLReader::finalize()
{
	for(int i = 0; i < (int)_add_targets.size(); i++) {
		_add_targets[i].execute(_flow,fmaps());
	}
	for(int i = 0; i < (int)_set_error_handlers.size(); i++) {
		_set_error_handlers[i].execute(_flow);
	}
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
	const char * el_virtual = NULL;
	const char * el_magicnumber = NULL;
	const char * el_wtype = NULL;
	const char * el_function = NULL;
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
		} else if(strcmp(attr[i],"virtual") == 0) {
			el_virtual = attr[i+1];
		} else if(strcmp(attr[i],"unionnumber") == 0) {
			el_unionnumber = attr[i+1];
		} else if(strcmp(attr[i],"inputunionnumber") == 0) {
			el_inputunionnumber = attr[i+1];
		} else if(strcmp(attr[i],"outputunionnumber") == 0) {
			el_outputunionnumber = attr[i+1];
		} else if(strcmp(attr[i],"magicnumber") == 0) {
			el_magicnumber = attr[i+1];
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
	bool is_virtual = (el_virtual ? strcmp(el_virtual,"true")==0 : false);
	int argno = (el_argno ? atoi(el_argno) : 0);
	int unionnumber = (el_unionnumber ? atoi(el_unionnumber) : 0);
	int inputunionnumber = (el_inputunionnumber ? atoi(el_inputunionnumber) : 0);
	int outputunionnumber = (el_outputunionnumber ? atoi(el_outputunionnumber) : 0);
	int magicnumber = (el_magicnumber ? atoi(el_magicnumber) : 0);
	int wtype = (el_wtype ? atoi(el_wtype) : 0);
	if(strcmp(el,"argument") == 0) {
		// has attributes: argno
		// has no children
		pthis->flow_guard_ref_add_argument(argno);
	} else if(strcmp(el,"guard") == 0) {
		// has attributes: name, magic_number
		// has no children 
		AtomicMapAbstract * amap = pthis->fmaps()->lookup_atomic_map(el_name);
		assert(amap);
		pthis->new_flow_guard(el_name, magicnumber, amap);
	} else if(strcmp(el,"guardref") == 0) {
		// has attributes: name, unionnumber, wtype
		// has children: argument(s)
		FlowGuard * fg = pthis->flow()->getGuard(el_name);
		assert(fg);
		pthis->new_flow_guard_reference(fg, unionnumber, wtype);
	} else if(strcmp(el,"condition") == 0) {
		// has attributes: name, argno, isnegated
		// has no children
		ConditionFn condfn = pthis->fmaps()->lookup_conditional(el_name,argno,unionnumber);
		assert(condfn != NULL);
		FlowCondition * fc = new FlowCondition(condfn,is_negated);
		pthis->flow_case()->add(fc);
	} else if(strcmp(el,"case") == 0) {
		// has attributes: nodetarget
		// has children: condition
		
		pthis->new_flow_case(el_nodetarget,pthis->flow_node()->outputUnionNumber(), is_virtual);
	} else if(strcmp(el,"successor") == 0) {
		// has no attributes
		// has children: case
		pthis->new_flow_successor();
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
		CreateNodeFn createfn = pthis->fmaps()->lookup_node_function(el_function);
		assert(createfn != NULL);
		pthis->new_flow_node(el_name, createfn, is_errorhandler, is_source, is_detached, inputunionnumber, outputunionnumber, is_virtual);
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

    // load attribute
	const char * el_file = NULL;

    // plugin attributes
	const char * el_external_node = NULL;
	const char * el_plugin_condition = NULL;
	const char * el_begin_node = NULL;

    // condition attributes
	const char * el_name = NULL;
	const char * el_argno = NULL;
	const char * el_isnegated = NULL;
	const char * el_unionnumber = NULL;
	const char * el_inputunionnumber = NULL;
	const char * el_outputunionnumber = NULL;

    // node attributes
	const char * el_function = NULL;
	const char * el_source = NULL;
	const char * el_iserrhandler = NULL;
	const char * el_detached = NULL;

    // case attribute
	const char * el_nodetarget = NULL;

    // guard attributes
	const char * el_magicnumber = NULL;
	const char * el_wtype = NULL;
    const char * el_external = NULL;

	for(int i = 0; attr[i]; i += 2) {
        // plugin attributes
        if(strcmp(attr[i],"file") == 0) {
			el_file = attr[i+1];
        } else if(strcmp(attr[i],"externalnode") == 0) {
			el_external_node = attr[i+1];
        } else if(strcmp(attr[i],"condition") == 0) {
			el_plugin_condition = attr[i+1];
        } else if(strcmp(attr[i],"beginnode") == 0) {
			el_begin_node = attr[i+1];
            // condition attributes
        } else if(strcmp(attr[i],"name") == 0) {
			el_name = attr[i+1];
		} else if(strcmp(attr[i],"argno") == 0) {
			el_argno = attr[i+1];
		} else if(strcmp(attr[i],"isnegated") == 0) {
			el_isnegated = attr[i+1];
		} else if(strcmp(attr[i],"unionnumber") == 0) {
			el_unionnumber = attr[i+1];
        } else if(strcmp(attr[i],"inputunionnumber") == 0) {
            el_inputunionnumber = attr[i+1];
        } else if(strcmp(attr[i],"outputunionnumber") == 0) {
            el_outputunionnumber = attr[i+1];
            // node attributes
		} else if(strcmp(attr[i],"function") == 0) {
			el_function = attr[i+1];
		} else if(strcmp(attr[i],"source") == 0) {
			el_source = attr[i+1];
		} else if(strcmp(attr[i],"iserrhandler") == 0) {
			el_iserrhandler = attr[i+1];
		} else if(strcmp(attr[i],"detached") == 0) {
            // case attribute
			el_detached = attr[i+1];
		} else if(strcmp(attr[i],"nodetarget") == 0) {
			el_nodetarget = attr[i+1];
            // guard attributes
		} else if(strcmp(attr[i],"magicnumber") == 0) {
			el_magicnumber = attr[i+1];
		} else if(strcmp(attr[i],"wtype") == 0) {
			el_wtype = attr[i+1];
		} else if(strcmp(attr[i],"external") == 0) {
			el_external = attr[i+1];
        }
    }

	bool is_negated = (el_isnegated ? strcmp(el_isnegated,"true")==0 : false);
	bool is_source = (el_source ? strcmp(el_source,"true")==0 : false);
	bool is_errorhandler = (el_iserrhandler ? strcmp(el_iserrhandler,"true")==0 : false);
	bool is_detached = (el_detached ? strcmp(el_detached,"true")==0 : false);
	bool is_external = (el_external ? strcmp(el_external,"true")==0 : false);

	int argno = (el_argno ? atoi(el_argno) : 0);
	int unionnumber = (el_unionnumber ? atoi(el_unionnumber) : 0);
	int inputunionnumber = (el_inputunionnumber ? atoi(el_inputunionnumber) : 0);
	int outputunionnumber = (el_outputunionnumber ? atoi(el_outputunionnumber) : 0);
	int magicnumber = (el_magicnumber ? atoi(el_magicnumber) : 0);
	int wtype = (el_wtype ? atoi(el_wtype) : 0);

	if(strcmp(el,"load") == 0) {
        // has attribute: file
        // has children: plugin
        pthis->loadplugin(el_file);
    } else if(strcmp(el,"plugin") == 0) {
        // has attribute: externalnode, condition, beginnode
        // has children: condition, node
        pthis->new_plugin(el_external_node, el_plugin_condition, el_begin_node);
    } else if(strcmp(el,"condition") == 0) { 
		// has attributes: name, argno, isnegated
		// has no children
		ConditionFn condfn;// = pthis->plugin_fmaps()->lookup_conditional(el_name,argno,unionnumber);
		//assert(condfn != NULL);
		FlowCondition * fc = new FlowCondition(condfn,is_negated);
        if(el_name == pthis->plugin()->conditionName()) {
            pthis->plugin()->condition(fc);
        } else {
            pthis->flow_case()->add(fc);
        }
    } else if(strcmp(el,"guard") == 0) {
        /* wait until plugin_maps is there
        AtomicMapAbstract * amap = pthis->plugin_fmaps()->lookup_atomic_map(el_name);
        assert(amap);
        pthis->new_plugin_guard(el_name, magicnumber, amap);
        */
    } else if(strcmp(el,"guardref") == 0) {
        /* wait until plugin_maps is there
        if(is_external) {
            // TODO: support external guardref 
        } else {
            FlowGuard * fg = pthis->plugin()->getGuard(el_name);
            assert(fg);
            pthis->new_plugin_guard_reference(fg, unionnumber, wtype);
        }
        */
    } else if(strcmp(el,"argument") == 0) {
        // has attributes: argno
        // had no children
        pthis->flow_guard_ref_add_argument(argno);
    } else if(strcmp(el,"node") == 0) { 
		// has attributes: name, function, source, iserrhandler, detached
		// has children: errorhandler, guardref(s), successorlist 
        CreateNodeFn createfn;// = pthis->plugin_fmaps()->lookup_node_function(el_function);
        //assert(createfn != NULL);
        pthis->new_flow_node(el_name, createfn, is_errorhandler, is_source, is_detached, inputunionnumber, outputunionnumber, false);
    } else if(strcmp(el,"successorlist") == 0) { 
        // has no attribute
        // has children: successor
        pthis->new_flow_successor_list(); // essentially do nothing
    } else if(strcmp(el,"successor") == 0) { 
        // has no attribute
        // has children: case
        pthis->new_flow_successor(); 
	} else if(strcmp(el,"case") == 0) {
		// has attributes: nodetarget
		// has children: condition
        // no virtual nodetarget allowed in a plugin
		pthis->new_flow_case(el_nodetarget, pthis->flow_node()->outputUnionNumber());
    }
}

void XMLReader::endPluginHandler(void *data, const char *el)
{
	XMLReader * pthis = static_cast<XMLReader *> (data);
	if(strcmp(el,"load") == 0) {
        // do nothing
    } else if(strcmp(el,"plugin") == 0) { 
        pthis->add_plugin();
    } else if(strcmp(el,"condition") == 0) { 
        // do nothing
	} else if(strcmp(el,"guard") == 0) {
		// do nothing
	} else if(strcmp(el,"guardref") == 0) {
        // wait until plugin_maps is there
		//pthis->complete_plugin_guard_reference();
	} else if(strcmp(el,"argument") == 0) {
		// do nothing
    } else if(strcmp(el,"node") == 0) { 
        pthis->add_plugin_node();
    } else if(strcmp(el,"successorlist") == 0) { 
        // do nothing essentially
        pthis->add_flow_successor_list(); 
    } else if(strcmp(el,"successor") == 0) { 
        pthis->add_flow_successor();
    } else if(strcmp(el,"case") == 0) { 
        pthis->add_flow_case();
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

void XMLReader::loadplugin(const char * filename)
{
    boost::shared_ptr<Library> plugin(new Library(filename));
    plugin->load();

    if(_plugin_fmaps) {
        delete _plugin_fmaps;
    }

    _plugin_fmaps = new FlowFunctionMaps(
        reinterpret_cast<ConditionalMap*>(plugin->getSymbol("_conditional_map")),
        reinterpret_cast<ModularCreateMap*>(plugin->getSymbol("_master_create_map")),
        reinterpret_cast<GuardTransMap*>(plugin->getSymbol("_theGuardTrans_map")),
        reinterpret_cast<AtomicMapMap*>(plugin->getSymbol("_atomic_map_map")),
        reinterpret_cast<IOConverterMap*>(plugin->getSymbol("_ioconverter_map"))
                                        );
}

void XMLReader::new_flow_case(const char * targetnodename, int node_output_unionnumber, bool is_virtual) 
{ 
    _flow_case = new FlowCase(NULL); 
    if(!is_virtual) {
        AddTarget at(_flow_case,targetnodename, node_output_unionnumber);
        _add_targets.push_back(at);
    } else {
        PluginMap::iterator plugin_itr = _plugins.find(targetnodename);
        assert(plugin_itr != _plugins.end());

        PluginMap::iterator lastPlugin = _plugins.upper_bound(targetnodename);
        for( ; plugin_itr != lastPlugin; ++plugin_itr) {

            Plugin * plugin = plugin_itr->second;

            // add plugin condition into flow case
            _flow_case->add(plugin->condition());

            AddTarget at(_flow_case, plugin->beginNodeName().c_str(), node_output_unionnumber);
            _add_targets.push_back(at);

            // add all nodes defined in plugin
            std::vector<FlowNode *>::iterator node_itr = plugin->getAllNodes().begin();
            for(; node_itr != plugin->getAllNodes().end(); ++node_itr) {
                _flow->add( *node_itr );
            }
        }

        // TODO: if there is a default action for virtual node, add a condition here
        //_flow_case->add( new OFluxCondition() );
    }
}

void XMLReader::add_flow_node() 
{ 
    if(!_flow_node->getIsVirtual()) {
        _flow->add(_flow_node);
    } else {
        const char * node_name = _flow_node->getName();
        PluginMap::iterator plugin_itr = _plugins.find(node_name);
        assert(plugin_itr != _plugins.end());

        // set each end node's successor_list with the virtual node's successor_list 
        PluginMap::iterator lastPlugin = _plugins.upper_bound(node_name);
        for( ; plugin_itr != lastPlugin; ++plugin_itr) {
            Plugin * plugin = plugin_itr->second;
            std::vector<FlowNode *> endNodes;
            plugin->getEndNodes(endNodes);
            std::vector<FlowNode *>::iterator node_itr = endNodes.begin();
            for(; node_itr != endNodes.end(); ++node_itr) {
                (*node_itr)->successor_list(_flow_node->successor_list());
            }
        }
    }

    _flow_node = NULL;
    _flow_successor_list = NULL;
}


};
