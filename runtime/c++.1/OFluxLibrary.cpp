#include "OFluxLibrary.h"
#include "boost/filesystem.hpp"
#include "OFlux.h"
#include "OFluxLogging.h"

namespace fs = boost::filesystem;

namespace oflux {

Library::Library( const std::string & path )
    : _path( path ), _handle( NULL )
{
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
    if( ! validatePath() )
        return false;
    ::dlerror();
    _handle = ::dlopen( _path.c_str(), mode );
    if( ! _handle ) {
        oflux_log_error("Library::load() dlerror: %s\n", ::dlerror());
        return false;
    }

    return true;
}

void * Library::getSymbol( const std::string & name )
{
    if( ! _handle ) {
        return NULL;
    }
    oflux_log_info("Library::getSymbol() attempt load of: %s\n", name.c_str());
    ::dlerror();
    void * res = ::dlsym( _handle, name.c_str() );
    if(!res) {
        oflux_log_error("Library::getSymbol() dlerror: %s\n", ::dlerror());
    }
    return res;
}

// private functions

bool Library::validatePath()
{    
    if( _path.empty() )
        return false;

    fs::path path( _path );

    if( ! fs::exists( path ) )
        return false;

    if( ! fs::is_regular( path ) )
        return false;

    return true;
}

} // namespace oflux 
