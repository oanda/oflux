#include "OFluxOrderable.h"
#include <cassert>
#include <string>
#include <map>

using namespace oflux;

class MS : public MagicSorter {
public:
        typedef std::map<std::string, MagicNumberable* > IMap;
        virtual MagicNumberable * getMagicNumberable(const char * c)
                { 
                IMap::iterator itr = _map.find(c);
                return (itr == _map.end() ? NULL : (*itr).second); 
                }

        bool validate() 
                {
                IMap::iterator itr = _map.begin();
                while(itr != _map.end()) {
                        assert((*itr).second->magic_number() > 0);
                        itr++;
                }
                return true;
                }
        inline void add(const char *c) 
                {
                std::string s = c;
                addPoint(c);
                _map[s] = new MagicNumberable();
                }

        IMap _map;
};

int main()
{
        MS ms;
        ms.add("aaaa");
        ms.add("bbbb");
        ms.numberAll();
        ms.validate();
        return 0;
}
