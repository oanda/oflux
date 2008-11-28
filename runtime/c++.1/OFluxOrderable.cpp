#include "OFluxOrderable.h"
#include <cassert>
#include <map>
#include <set>

namespace oflux {

void MagicSorter::addInequality(const char * before, const char * after)
{
        Inequality ineq(getMagicNumberable(before),getMagicNumberable(after));
        _inequalities.push_back(ineq);
}

static bool all_magicnumber_assigned(std::set<MagicNumberable *> & s)
{
        bool res = true;
        std::set<MagicNumberable *>::iterator itr = s.begin();
        while(itr != s.end() && res) {
                res = res && ((*itr)->magic_number() > 0);
                itr++;
        }
        return res;
}

void MagicSorter::numberAll()
{
        // initially they are all numbered 0
        // job is to assign them integer magic numbers > 0
        int magic = 1;
        std::map<MagicNumberable *, std::set<MagicNumberable *> > pred_map;
        for(int i = 0; i < (int) _inequalities.size(); i++) {
                std::map<MagicNumberable *, std::set<MagicNumberable *> >::iterator aft_itr = pred_map.find (_inequalities[i].after);
                if(aft_itr == pred_map.end()) {
                        std::set<MagicNumberable *> empty;
                        std::pair<MagicNumberable *, std::set<MagicNumberable *> > pr(_inequalities[i].after,empty);
                        aft_itr = pred_map.insert(pr).first;
                }
                (*aft_itr).second.insert(_inequalities[i].before);
                std::map<MagicNumberable *, std::set<MagicNumberable *> >::iterator bef_itr = pred_map.find (_inequalities[i].before);
                if(bef_itr == pred_map.end()) {
                        std::set<MagicNumberable *> empty;
                        std::pair<MagicNumberable *, std::set<MagicNumberable *> > pr(_inequalities[i].before,empty);
                        pred_map.insert(pr);
                }
        }

        bool nochange = false;
        bool allassigned = false;
        while(!nochange) {
                nochange = true;
                allassigned = true;
                std::map<MagicNumberable *, std::set<MagicNumberable *> >::iterator itr = pred_map.begin();
                while(itr != pred_map.end()) {
                        allassigned = allassigned 
                                && ((*itr).first->magic_number() > 0);
                        if((*itr).first->magic_number() == 0
                                        && all_magicnumber_assigned((*itr).second)) {
                                (*itr).first->magic_number(magic++);
                                nochange = false;
                        }
                        itr++;
                }
        }
        assert(allassigned);
}


} // namespace
