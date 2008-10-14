#include "OFluxXML.h"
#include "OFluxFlow.h"
#include <fstream>
#include <expat.h>
#include <cassert>

namespace oflux {

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
                assert(_fc->ioConverter() == NULL);
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

XMLReader::XMLReader(const char * filename, FlowFunctionMaps *fmaps)
	: _fmaps(fmaps)
	, _flow(new Flow())
{ 
	read(filename); 
}


void XMLReader::read(const char * filename)
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
	XML_SetElementHandler(p, XMLReader::startHandler, XMLReader::endHandler);
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
	finalize();
	XML_ParserFree(p);
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

void XMLReader::startHandler(void *data, const char *el, const char **attr)
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
		
		pthis->new_flow_case(el_nodetarget,outputunionnumber);
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
		pthis->new_flow_node(el_name, createfn, is_errorhandler, is_source, is_detached,inputunionnumber,outputunionnumber);
	}
}

void XMLReader::endHandler(void *data, const char *el)
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

void XMLReader::dataHandler(void *data, const char *xml_data, int len)
{
	// not used
}

void XMLReader::commentHandler(void *data, const char *comment)
{
	// not used
}



};
