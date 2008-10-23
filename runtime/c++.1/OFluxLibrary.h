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
    void * getSymbol( const std::string & path );

private:
    Library();
    Library( const Library & );
    bool validatePath();

    std::string path_;
    void * handle_;

}; // class Library

} // namespace oflux 

#endif // _OFLUX_LIBRARY
