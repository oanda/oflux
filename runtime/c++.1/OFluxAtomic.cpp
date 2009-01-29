#include "OFluxAtomic.h"

namespace oflux {

boost::shared_ptr<EventBase> Atomic::_null_static; // null

void * AtomicPool::get_data()
{
        void * ptr = NULL;
        if(_dq.size()) {
                ptr = _dq.front();
                _dq.pop_front();
        }
        return ptr;
}
void AtomicPool::put_data(void * p)
{
        assert(p);
        _dq.push_back(p);
}

boost::shared_ptr<EventBase> AtomicPool::get_waiter()
{
        boost::shared_ptr<EventBase> r;
        if(_q.size()) {
                r = _q.front();
                _q.pop_front();
        }
        return r;
}
void AtomicPool::put_waiter(boost::shared_ptr<EventBase> & ev)
{
        _q.push_back(ev);
}
const void * AtomicPool::get(Atomic * & a_out,const void * key) 
{
        static int _to_use;
        //assert(_ap_list == NULL || _ap_list->next() != _ap_list);
        if(_ap_list) {
                a_out = _ap_list;
                _ap_list = _ap_list->next();
                reinterpret_cast<AtomicPooled *>(a_out)->next(NULL);
        } else {
                a_out = new AtomicPooled(*this,NULL);
        }
        //assert(_ap_list == NULL || _ap_list->next() != _ap_list);
        assert(*(a_out->data()) == NULL);
        //assert(a_out != _ap_list);
        return &_to_use;
}

void AtomicPool::put(AtomicPooled * ap)
{
        //assert(_ap_list == NULL || _ap_list->next() != _ap_list);
        //assert(ap != _ap_list);
        assert(*(ap->data()) == NULL);
        ap->next(_ap_list);
        _ap_list = ap;
        //assert(_ap_list == NULL || _ap_list->next() != _ap_list);
}

AtomicPool::~AtomicPool()
{
        AtomicPooled * next;
        AtomicPooled * on = _ap_list;
        while(on) {
                next = on->next();
                delete on;
                on = next;
        }
}

bool AtomicPoolWalker::next(const void * & key, Atomic * &atom)
{
        bool res = (_n != NULL);
        key = NULL;
        if(res) {
                atom = _n;
                _n = _n->next();
        }
        return res;
}

bool TrivialWalker::next(const void * & key, Atomic * &atom)
{
        bool res = _more;
        if(res) {
                key = NULL;
                atom = &_atom;
                _more = false;
        }
        return res;
}

};
