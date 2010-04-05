#ifndef OFLUX_FLOW_COMMON
#define OFLUX_FLOW_COMMON

#include <vector>
#include <map>
#include <deque>

namespace oflux {
namespace flow {

template<class O>
inline void 
delete_vector_contents(std::vector<O *> & vo)
{
        for(int i = 0; i < (int) vo.size(); i++) {
                delete vo[i];
        }
}

template<class O>
inline void 
delete_vector_contents_reversed(std::vector<O *> & vo)
{
        for(int i = ((int)vo.size())-1; i >= 0 ; --i) {
                delete vo[i];
        }
}

template<class O>
inline void 
delete_deque_contents(std::deque<O *> & mo)
{
        typename std::deque<O *>::iterator itr;
        itr = mo.begin();
        while(itr != mo.end()) {
                delete (*itr);
                itr++;
        }
}

template<typename K, typename O>
inline void 
delete_map_contents(std::map<K,O *> & mo)
{
        typename std::map<K,O *>::iterator mitr;
        mitr = mo.begin();
        while(mitr != mo.end()) {
                delete (*mitr).second;
                mitr++;
        }
}

} // namespace flow
} // namespace oflux


#endif // OFLUX_FLOW_COMMON
