#include "OFluxPlugin.h"
#include "boost/filesystem.hpp"

namespace fs = boost::filesystem;

namespace oflux {

Plugin::Plugin( const std::string & path )
    : path_( path ), handle_( NULL )
{
}

Plugin::~Plugin()
{
    if( handle_ )
    {
        ::dlclose( handle_ );
        handle_ = NULL;
    }
}

bool Plugin::load( int mode )
{
    if( ! validatePath() )
        return false;

    handle_ = ::dlopen( path_.c_str(), mode );
    if( ! handle_ )
        return false;

    return true;
}

void * Plugin::getSymbol( const std::string & path )
{
    if( ! handle_ )
        return NULL;

    return ::dlsym( handle_, path.c_str() );
}

// private functions

bool Plugin::validatePath()
{    
    if( path_.empty() )
        return false;

    fs::path path( path_ );

    if( ! fs::exists( path ) )
        return false;

    if( ! fs::is_regular( path ) )
        return false;

    return true;
}

} // namespace oflux 
