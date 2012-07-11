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
#include "flow/OFluxFlowLibrary.h"
#include "OFlux.h"
#include "OFluxLogging.h"

namespace oflux {
namespace flow {

static void trim_extension_and_prefix(std::string & str)
{
        size_t dotpos = str.find_last_of('.');
        if(dotpos != std::string::npos) {
                str.replace(dotpos,3,""); // trim ".so" or whatever
        }
        size_t libpos = str.find("lib",0,3);
        if(libpos != std::string::npos) {
                str.replace(libpos,3,""); // trim the lib prefix if there
        }
}

Library::Library( const char * path, const char * filename )
        : _path( path )
        , _filename( filename )
        , _name(filename)
        , _handle( NULL )
{
        trim_extension_and_prefix(_name);
}

Library::~Library()
{
        if( _handle )
        {
		deinit();
                ::dlclose( _handle );
                _handle = NULL;
        }
}

void
Library::init(void * init_plugin_params)
{
	std::string initfunction = "init__";
	addSuffix(initfunction);
	InitFunction * initfunc =
		getSymbol<InitFunction>(initfunction.c_str(), true); // ignore dlerror -- cause the programmer might not define it
	if(initfunc) {
		(*initfunc)(init_plugin_params);
	}
}

void
Library::deinit()
{
	std::string deinitfunction = "deinit__";
	addSuffix(deinitfunction);
	DeInitFunction * deinitfunc =
		getSymbol<DeInitFunction>(deinitfunction.c_str(), true); // ignore dlerror -- cause the programmer might not define it
	if(deinitfunc) {
		(*deinitfunc)();
	}
}

bool Library::load( int mode )
{
        std::string full_path(_path);
        full_path = full_path + "/" + _filename;
        ::dlerror();
        _handle = ::dlopen( full_path.c_str(), mode );
        if( ! _handle ) {
                oflux_log_error("Library::load() dlerror: %s\n", ::dlerror());
                return false;
        }

        return true;
}

void Library::addSuffix(std::string & str)
{
        /*
        if(_filename.length() > 3) {
                str += _filename.c_str() + 3; // drop "lib"
        }
        trim_extension(str); // drop the ".so"
        */
        str += _name;
}

void * Library::_getSymbol( const char * name, bool ignoreError )
{
        if( ! _handle ) {
                return NULL;
        }
        oflux_log_info("Library::_getSymbol() attempt load of: %s\n", name);
        ::dlerror();
        void * res = ::dlsym( _handle, name );
        if(!res && !ignoreError) {
                oflux_log_error("Library::_getSymbol() dlerror: %s\n", ::dlerror());
        }
        return res;
}
} // namespace flow
} // namespace oflux 
