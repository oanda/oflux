#ifndef OFLUX_LOCKFREE_MEMPOOL_H
#define OFLUX_LOCKFREE_MEMPOOL_H

/**
 * @file OFluxLFMemoryPool.h
 * @author Mark Pichora
 *  Lock-free memory allocation pools.  Does its best to memory pool on
 * a per-thread basis.
 */

#include <cstdlib>
#include <new>
#include <malloc.h>
#include <pthread.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include "lockfree/allocator/OFluxMetaLog.h"
#include "lockfree/OFluxMachineSpecific.h"
#include "lockfree/OFluxThreadNumber.h"

namespace oflux {
namespace lockfree {
namespace allocator {

template<size_t el_sz>
class RawMemPoolElement {
public:
        struct El {
                El * next;
        };
	union ElStorage { char raw[el_sz]; El e; };
};

template<size_t el_sz>
class RawMemPoolElementList { 
public:
        // A contention-free single reader, single writer queue
        //
        typedef typename RawMemPoolElement<el_sz>::El El;

        RawMemPoolElementList()
                : _head(NULL)
                , _tail(NULL)
        {}
        inline bool 
	push(El * e) {
                bool res = (_head == NULL);
		//El * old_h = _head;
                e->next = _head;
                _head = e;
		if(res) { // only done on startup really
			_tail = _head;
		}
		//assert(_head->next != _head);
		//assert(_head->next != NULL || _tail == _head);
                return res;
        }
        inline El * 
	pull(El * & tt) {
                El * h= _head;
                El * hn = (h ? h->next : NULL);
                if(h) {
                        h->next = NULL;
                }
                tt = _tail;
                _tail = h;
                return hn;
        }

        El * _head;
        El * _tail;
};


template<size_t sz, size_t el_sz, size_t offset=0>
class RawMemPool {
public:
        typedef typename RawMemPoolElement<el_sz>::El El;
	typedef typename RawMemPoolElement<el_sz>::ElStorage ElStorage;

	enum {    opt_sz = 
			((1 << Log<sz>::result) * sizeof(ElStorage)
				- sizeof(size_t) 
				- sizeof(El *)
				- offset) 
			/ sizeof(ElStorage)
		, opt_pad = 
			((2+offset)*sizeof(ElStorage)
				- sizeof(size_t)
				- sizeof(El *)
				- offset) % sizeof(ElStorage) 
		};


	RawMemPool()
		: _horizon(0)
		, _head(NULL)
		{}

#define check_assert(H) \
	assert((H) == NULL || ( (H) >= &(_raw[0].e) && (H) < &(_raw[opt_sz].e) ))

	inline El * 
	get() {
		El * res = NULL;
		//assert(_head ==NULL || _head->next != _head);
		if(_head) {
                        res = _head;
			//assert(_head->next != _head);
                        _head = _head->next;
			//check_assert(_head);
                } else {
                        if(_horizon < opt_sz) {
                                res = &(_raw[_horizon].e);
                                ++_horizon;
                        }
                }
		//assert(_head ==NULL || _head->next != _head);
                return res;
	}
	inline void 
	put(El *m) {
		//assert(_head ==NULL || _head->next != _head);
                if( m >= &(_raw[0].e) && m < &(_raw[opt_sz].e) ) {
                        m->next = _head;
                        _head = m;
			//assert(_head->next != _head);
                } else {
                        assert( 0 && "attempt to return memory -- not mine" );
                }
		//assert(_head ==NULL || _head->next != _head);
        }
        inline void 
	put(El * h, El * t) {
		//assert(_head ==NULL || _head->next != _head);
                t->next = _head;
                _head = h;
		//El * hh = h;
		//while(hh!= NULL && hh != hh->next) {
			//hh = hh->next;
		//}
		//assert(hh==NULL);
		//assert(_head ==NULL || _head->next != _head);
        }
	void 
	verify_full(int index) {
		size_t comp_sz = (opt_sz - _horizon);
		El * h = _head;
		while(h) {
			++comp_sz;
			h = h->next;
		}
		printf("mempool (thread %d) has sz %u and now has %u in it\n"
			, index, opt_sz, comp_sz);
	}
private:
	size_t    _horizon;
	El *      _head;
	char      _pad[opt_pad];
	ElStorage _raw[opt_sz];
};

#define DEFAULT_MEMPOOL_NUM_ELEMENTS (8*1024)

template< size_t el_sz
        , size_t num_threads =DEFAULT_MEMPOOL_MAX_THREADS
        , size_t num_elements=DEFAULT_MEMPOOL_NUM_ELEMENTS >
class MemoryPool : public oflux::AllocatorImplementation {
public:
	typedef typename RawMemPool<num_elements,el_sz>::El El;

        MemoryPool() 
        {
                // put one element in each to avoid races
                for(size_t i = 0; i < num_threads; ++i) {
                        for(size_t j = 0; j < num_threads; ++j) {
                                _pt[j].qus[i].push(_pt[i].arr.get());
                        }
                }
        }
	~MemoryPool()
	{
                for(size_t i = 0; i< num_threads; ++i) {
                        for(size_t j = 0; j < num_threads; ++j) {
				if(_pt[j].qus[i]._head) {
					_pt[i].arr.put(
						  _pt[j].qus[i]._head
						, _pt[j].qus[i]._tail);
				}
			}
		}
		const char * memp_debug = getenv("MEMPOOL_DEBUG");
		if(memp_debug && strcmp(memp_debug,"check") == 0) {
			for(size_t i = 0; i < (size_t)ThreadNumber::num_threads; ++i) {
				_pt[i].arr.verify_full(i);
			}
		}
	}
        /**
         * @brief index indicates your current thread
         */ 
        virtual void * 
	get() {
		size_t index = oflux::lockfree::_tn.index;
                void * res = _pt[index].arr.get();
                if(!res && collect(index)) {
                        res = _pt[index].arr.get();
			assert(!(res < &_pt[0] || res >= &_pt[num_threads]));
                } else if(!res) {
                        res = malloc(el_sz);
			if(!res) throw std::bad_alloc();
			//++malloc_counter;
			assert(res < &_pt[0] || res >= &_pt[num_threads]);
                }
		assert(res);
		return res;
        }
        /**
         * @brief index indicates your current thread
         */ 
        virtual void 
	put(void *m)     // the element itself
	{
		if(!m) return;
		size_t index = oflux::lockfree::_tn.index; // the FROM index
                El * e = static_cast<El *>(m);
                int mem_index = calc_mem_index(m); // to TO index
                if(mem_index >= 0) {
                        if((int)index == mem_index) {
                                // off load it directly
                                _pt[index].arr.put(e);
                        } else {
                                // put it in the mail
				bool pres = _pt[index].qus[mem_index].push(e);
                                _pt[mem_index].collect_touch = 
                                        _pt[mem_index].collect_touch && pres;
                        }
			assert(!(m < &_pt[0] || m >= &_pt[num_threads]));
                } else { // memory is from heap
			assert(m < &_pt[0] || m >= &_pt[num_threads]);
			//++free_counter;
                        free(m);
                }
        }
protected:
        /**
         * @brief collect memory elements contributed back by other threads
         * @param index is the thread index who is trying to pull out more
         *        memory elements
         */
        inline bool 
	collect(size_t index) // given the index to collect 
	{
                if(_pt[index].collect_touch) {
                        return false;
                }
		//++collect_counter;
                El * h;
                El * t;
                bool res = false;
                for(size_t i = 0; i < ThreadNumber::num_threads; ++i) {
                        h = _pt[i].qus[index].pull(t);
                        if(h) {
                                _pt[index].arr.put(h,t);
                                res = true;
                        }
                }
                _pt[index].collect_touch = true;
                return res;
        }
	inline int 
	calc_mem_index(void *m)
	{
		void * mp_start = &_pt[0];
		void * mp_end = &_pt[num_threads];
		return (m >= mp_end || m < mp_start 
			? -1 
			: DivideBy<sizeof(_pt[0])>::into(
				(size_t)(m) - (size_t)(mp_start)));
	}
private:
	struct PerThreadBase0 {
					// The list of element lists for elements
					// FROM this thread TO the indexed (i-th)
					// thread
		RawMemPoolElementList<el_sz>   qus[num_threads];
	};
	struct PerThreadBase1 : public PerThreadBase0 {
		char _pad1[(oflux::lockfree::MachineSpecific::Cache_Line_Size*10 
				- sizeof(PerThreadBase0))%oflux::lockfree::MachineSpecific::Cache_Line_Size];
					// indicate that this thread should
					// bother to collect()
		bool                           collect_touch;     
		char _pad2[(oflux::lockfree::MachineSpecific::Cache_Line_Size - sizeof(bool))%oflux::lockfree::MachineSpecific::Cache_Line_Size];
	};
	struct PerThread : public PerThreadBase1 {
		enum { size_offset = sizeof(PerThreadBase1) };

					// the private memory pool for 
					// 	this thread
		RawMemPool<num_elements,el_sz,size_offset> arr;
	};
	PerThread _pt[num_threads];     // the per thread structure indexed

};



extern void init_ThreadNumber(ThreadNumber & tn);

} // namespace allocator
} // namespace lockfree
} // namespace oflux

#endif // OFLUX_LOCKFREE_MEMPOOL_H
