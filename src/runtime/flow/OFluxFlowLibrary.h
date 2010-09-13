#ifndef _OFLUX_FLOW_LIBRARY
#define _OFLUX_FLOW_LIBRARY

/**
 * @file OFluxFlowLibrary
 * @author Mark Pichora
 * Wrapper for dynamically loaded libraries (which are used to implement 
 * plugins).  The top-level flow manages these and ensures that they are
 * loaded before the XML is read into the flow data structure.
 */ 

#include <string>
#include <vector>
#include <dlfcn.h>

namespace oflux {

namespace flow {

class Flow;

class Library {
public:
	typedef Flow ParentObjType;

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

	/** @brief initialize the library (call init__...())
	 */
	void init(void * init_plugin_params);

	/** @brief de-initialize the library (call deinit__...())
	 */
	void deinit();

	const std::vector<std::string> & getDependencies() 
	{ return _dependencies; }
	void addDependency(const char * d)
	{ std::string s(d); _dependencies.push_back(s); }

private:
        void * _getSymbol(const char * name, bool ignoreError);
        Library();
        Library( const Library & );

        std::string _path;
        std::string _filename;
        std::string _name;
        void * _handle;
	std::vector<std::string> _dependencies;

}; // class Library

} // namespace flow

} // namespace oflux 

#endif // _OFLUX_LIBRARY
