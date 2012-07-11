#ifndef _OFLUX_FLOW_LIBRARY
#define _OFLUX_FLOW_LIBRARY
/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file OFluxFlowLibrary.h
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
        virtual ~Library();
        
        /** @brief load the library 
         */
        virtual bool load( int mode = RTLD_NOW | RTLD_GLOBAL );

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

protected:
        virtual void * _getSymbol(const char * name, bool ignoreError);
private:
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
