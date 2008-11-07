#include "OFluxLibrary.h"
#include "boost/filesystem.hpp"

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

    _handle = ::dlopen( _path.c_str(), mode );
    if( ! _handle )
        return false;

    return true;
}

void * Library::getSymbol( const std::string & name )
{
    if( ! _handle )
        return NULL;

    return ::dlsym( _handle, name.c_str() );
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
