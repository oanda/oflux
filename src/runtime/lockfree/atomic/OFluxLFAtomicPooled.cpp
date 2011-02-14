#include "lockfree/atomic/OFluxLFAtomicPooled.h"
#include "lockfree/allocator/OFluxSMR.h"
#include <cstdio>

namespace oflux {
namespace lockfree {
namespace atomic {


// clarify when _resource_ebh is what:
//------------------------------------
//
// constructed:  emptied ebh (has resource set to NULL)
// acquired:   populated ebh (resource set to not NULL)
// released:
//


AtomicPooled::AtomicPooled(AtomicPool * pool,void * data)
	: _next(NULL)
	, _pool(pool)
	, _resource_ebh(AtomicCommon::allocator.get(data))
	, _by_ebh(AtomicCommon::allocator.get('\0',&_resource_ebh))
{
	oflux_log_trace2("AC::alloc.g APD*this:%p _resource_ebh:%p %d\n"
		, this
		, _resource_ebh
		, __LINE__);
	oflux_log_trace2("AC::alloc.g APD*this:%p _by_ebh:%p %d\n"
		, this
		, _by_ebh
		, __LINE__);
	init();
	oflux_log_trace2("APD::APD APD*this:%p\n",this);
	//check();
}

void
AtomicPooled::relinquish()
{
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] APD::relinquish APD*this:%p\n"
		, oflux_self()
		, this);
	_pool->release(this);
	//check();
}

const char *
AtomicPooled::atomic_class_str = "lockfree::Pooled";

bool
AtomicPooled::acquire_or_wait(EventBasePtr & ev,int t)
{
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	assert(_by_ebh);
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AP::a_o_w ev %s ev*:%p  atom APD*this:%p "
		" _by_ebh:%p _by_ebh->rsrc_loc: %p\n"
		, oflux_self()
		, ev->flow_node()->getName()
		, ev.get()
		, this
		, _by_ebh
		, _by_ebh->resource_loc);
	EventBase * evb = ev.get();
	_by_ebh->ev = evb;
	_by_ebh->type = t;
	if(_resource_ebh) {
		assert(_resource_ebh->resource == NULL);
		oflux_log_trace2("AC::alloc.p _resource_ebh:%p %d\n"
			, _resource_ebh
			, __LINE__);
		AtomicCommon::allocator.put(_resource_ebh);
		_resource_ebh = NULL;
	}
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	EventBaseHolder * local_by_ebh = _by_ebh;
	_by_ebh = AtomicCommon::allocator.get('\0',&_resource_ebh);
	oflux_log_trace2("AC::alloc.g %p %d\n",_by_ebh,__LINE__);
	init();
	bool acqed = _pool->waiters->acquire_or_wait(local_by_ebh); // try to acquire the resource
	assert(!acqed || _resource_ebh->resource);
	if(acqed) {
		AtomicCommon::allocator.put(local_by_ebh);
	} else {
		assert(ev.recover());
	}
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AP::a_o_w ev %s ev*:%p APD*this:%p res %d\n"
		, oflux_self()
		, evb->flow_node()->getName()
		, evb
		, this
		, acqed);
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	return acqed;
}

void
AtomicPooled::release(
	  std::vector<EventBasePtr> & rel_ev_vec
	, EventBasePtr &ev)
{
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	assert(_resource_ebh);
	assert(_resource_ebh->resource != NULL);
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AP::rel   ev %s ev*:%p  APD*this:%p _by_ebh:%p _res_ebh->res:%p\n"
		, oflux_self()
		, ev ? ev->flow_node()->getName() : "<NULL>"
		, ev
		, this
		, _by_ebh
		, _resource_ebh->resource);
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	EventBaseHolder * rel_ebh = _pool->waiters->release(_by_ebh);
	//assert(_by_ebh->resource_loc == &_resource_ebh);
	if(rel_ebh) { 
		assert(rel_ebh != _by_ebh);
		assert(rel_ebh->ev);
		assert((*(rel_ebh->resource_loc))->resource);
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AP::rel   ev %s ev*:%p APD*this:%p passed to %s rel_ebh->ev:%p rel_ebh->res_loc:%p rsrc:%p\n"
			, oflux_self()
			, ev.get() ? ev->flow_node()->getName() : "<NULL>"
			, ev.get()
			, this
			, rel_ebh->ev->flow_node()->getName()
			, rel_ebh->ev
			, rel_ebh->resource_loc
			, (*(rel_ebh->resource_loc))->resource);
		//_by_ebh.ev = rel_ebh->ev;
		//_by_ebh.type = rel_ebh->type;
		rel_ev_vec.push_back(EventBasePtr(rel_ebh->ev));
		oflux_log_trace2("AC::alloc.p rel_ebh:%p %d\n"
			, rel_ebh
			, __LINE__);
		AtomicCommon::allocator.put(rel_ebh);
	} else {
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AP::rel   ev %s ev*:%p  APD*this:%p rsrc nothing out\n"
			, oflux_self()
			, ev.get() ? ev->flow_node()->getName() : "<NULL>"
			, ev.get()
			, this);
	}
	static int * _null = NULL;
	_resource_ebh = AtomicCommon::allocator.get(_null);
	store_load_barrier();
	oflux_log_trace2("AC::alloc.g _res_ebh:%p %d\n",_resource_ebh,__LINE__);
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	if(!rel_ebh) {
		_pool->release(this);
	}
}

Allocator<AtomicPooled,DeferFree> AtomicPool::allocator; // (new allocator::MemoryPool<sizeof(AtomicPooled)>());

AtomicPool::AtomicPool()
	: waiters(new PoolEventList())
	, head_free(NULL)
{
}

AtomicPool::~AtomicPool()
{
	oflux_log_trace2("~AtomicPool APD*this:%p\n",this);
	// reclaim items from the free list
	AtomicPooled * aph = head_free;
	AtomicPooled * aph_n = NULL;
	while(aph) {
		aph_n = aph->_next;
		AtomicPool::allocator.put(aph);
		aph = aph_n;
	}
	delete waiters;
}

void
AtomicPool::release(AtomicPooled *ap)
{
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AP::release APD*:%p\n",oflux_self(),ap);
	ap->init();
	// put it on the free list
	AtomicPooled * h;
	do {
		h = head_free;
		ap->_next = h;
	} while(!__sync_bool_compare_and_swap(&head_free,h,ap));
}

const void *
AtomicPool::get(oflux::atomic::Atomic * & a_out,const void *)
{
	char _here;
	static const char * p_here = &_here;
	AtomicPooled * ap_out = NULL;
	while((ap_out = head_free) 
			&& !__sync_bool_compare_and_swap(
				  &head_free
				, ap_out
				, ap_out->_next));
	if(!ap_out) {
		void * p = NULL;
		ap_out = AtomicPool::allocator.get(this,p);
		oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AP::get APD*:%p new\n",oflux_self(),ap_out);
	} else {
		oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AP::get APD*:%p old\n",oflux_self(),ap_out);
	}
	a_out = ap_out;
	//ap_out->check();
	return &p_here;
}

void
AtomicPooled::check()
{
	assert(_by_ebh->resource_loc == &_resource_ebh);
}

class AtomicPoolWalker : public oflux::atomic::AtomicMapWalker {
public:
	AtomicPoolWalker(AtomicPool &pool)
		: _done(false)
		, _atomic(&pool,NULL)
	{}
	virtual ~AtomicPoolWalker();
	virtual bool next(const void * & key,oflux::atomic::Atomic * &atom);
	// so we pretend there is only one atomic
	// this is cool for most applications of this walker -- I think
private:
	bool _done;
	AtomicPooled _atomic;
};

AtomicPoolWalker::~AtomicPoolWalker()
{
}

bool 
AtomicPoolWalker::next(const void * &, oflux::atomic::Atomic * &atom)
{
	bool res = !_done;
	atom = NULL;
	if(res) {
		_done = true;
		atom = &_atomic;
	}
	return res;
}

oflux::atomic::AtomicMapWalker *
AtomicPool::walker()
{
	return new AtomicPoolWalker(*this);
}

} // namespace atomic
} // namespace lockfree
} // namespace oflux

