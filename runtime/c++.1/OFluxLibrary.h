#ifndef _OFLUX_LIBRARY
#define _OFLUX_LIBRARY

#include <string>
#include <dlfcn.h>

namespace oflux {

class Library
{
public:
    Library( const std::string & path );
    ~Library();
    
    bool load( int mode = RTLD_NOW ); 
    void * getSymbol( const std::string & name );

private:
    Library();
    Library( const Library & );
    bool validatePath();

    std::string _path;
    void * _handle;

}; // class Library

} // namespace oflux 

#endif // _OFLUX_LIBRARY
