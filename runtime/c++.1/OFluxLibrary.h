#ifndef _OFLUX_LIBRARY
#define _OFLUX_LIBRARY

#include <string>
#include <dlfcn.h>

namespace oflux {

namespace flow {

class Library
{
public:
        Library( const char * path, const char * filename );
        ~Library();
        
        /** @brief load the library 
         */
        bool load( int mode = RTLD_NOW | RTLD_GLOBAL );

        /** @brief grab a typed symbol from the library
         */
        template<typename T>
        T * getSymbol(const char * name, bool ignoreError = false)
        {
                return reinterpret_cast<T *>(_getSymbol(name, ignoreError));
        }

        /** @brief add a unique simple suffix to the given str based 
         * on lib name
         */
        void addSuffix(std::string & str);

        const std::string & getName() const { return _name; }

private:
        void * _getSymbol(const char * name, bool ignoreError);
        Library();
        Library( const Library & );

        std::string _path;
        std::string _filename;
        std::string _name;
        void * _handle;

}; // class Library

} // namespace flow

} // namespace oflux 

#endif // _OFLUX_LIBRARY
