#include "OFluxLibrary.h"
#include "OFlux.h"
#include "OFluxLogging.h"

namespace oflux {

static void trim_extension(std::string & str)
{
        size_t dotpos = str.find_last_of('.');
        if(dotpos != std::string::npos) {
                str.replace(dotpos,3,""); // trim ".so" or whatever
        }
}

Library::Library( const char * path, const char * filename )
        : _path( path )
        , _filename( filename )
        , _name(filename)
        , _handle( NULL )
{
        trim_extension(_name);
}

Library::~Library()
{
        if( _handle )
        {
                ::dlclose( _handle );
                _handle = NULL;
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
        if(_filename.length() > 3) {
                str += _filename.c_str() + 3; // drop "lib"
        }
        trim_extension(str); // drop the ".so"
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

} // namespace oflux 
