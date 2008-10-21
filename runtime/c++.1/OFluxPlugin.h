#ifndef _OFLUX_PLUGIN
#define _OFLUX_PLUGIN

#include <string>
#include <dlfcn.h>

namespace oflux {

class Plugin
{
public:
    Plugin( const std::string & path );
    ~Plugin();
    
    bool load( int mode = RTLD_NOW ); 
    void * getSymbol( const std::string & path );

private:
    Plugin();
    Plugin( const Plugin & );
    bool validatePath();

    std::string path_;
    void * handle_;

}; // class Plugin

} // namespace oflux 

#endif // _OFLUX_PLUGIN
