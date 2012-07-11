#ifndef _OFLUX_XML
#define _OFLUX_XML
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

#include <string>

/**
 * @file OFluxXML.h
 * @author Mark Pichora
 * This is an XML reader class that understands XML files produced by oflux
 */

namespace oflux {
 class PluginSourceAbstract;
namespace flow {
 class FunctionMapsAbstract;
 class Flow;
 class Library;
} // namespace flow
namespace xml {

/**
 * @class ReaderException
 * @brief exception class for oops-es in parsing etc
 */
class ReaderException : public std::exception {
public:
        ReaderException(const char * mesg)
        		: _mesg(mesg)
        {}

        virtual ~ReaderException() throw ()
        {}

        virtual const char* what () const throw () {
            return _mesg.c_str();
        }
private:
        std::string _mesg;
};

flow::Library * flow__Library_factory(const char * dir, const char * name);

/**
 * @brief read a flow from an XML file
 * @param filename of the xml file
 * @param fmaps the compiled-in symbol mappings
 * @param directory of XML plugin files
 * @param directory of the .so plugin files
 * @param plugin parameters data object
 * @param existing_flow that was loaded previously
 * @param atomics_style indicates the rev. of the guard structures used by
 *        this runtime
 * @returns a flow object
 **/
flow::Flow *
read(     const char * filename
	, flow::FunctionMapsAbstract *fmaps
	, PluginSourceAbstract * pluginxmldir
	, const char * pluginlibdir
	, void * initpluginparams
	, const flow::Flow * existing_flow
	, int atomics_style
        );

} // namespace xml
} // namespace oflux



#endif // _OFLUX_XML
