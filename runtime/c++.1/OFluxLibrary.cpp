#include "OFluxLibrary.h"
#include "boost/filesystem.hpp"

namespace fs = boost::filesystem;

namespace oflux {

Library::Library( const std::string & path )
    : path_( path ), handle_( NULL )
{
}

Library::~Library()
{
    if( handle_ )
    {
        ::dlclose( handle_ );
        handle_ = NULL;
    }
}

bool Library::load( int mode )
{
    if( ! validatePath() )
        return false;

    handle_ = ::dlopen( path_.c_str(), mode );
    if( ! handle_ )
        return false;

    return true;
}

void * Library::getSymbol( const std::string & path )
{
    if( ! handle_ )
        return NULL;

    return ::dlsym( handle_, path.c_str() );
}

// private functions

bool Library::validatePath()
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
