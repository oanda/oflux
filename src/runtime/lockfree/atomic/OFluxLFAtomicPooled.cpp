#include "lockfree/atomic/OFluxLFAtomicPooled.h"
#include "lockfree/allocator/OFluxSMR.h"
#include <cstdio>

namespace oflux {
namespace lockfree {
namespace atomic {




AtomicPooled::AtomicPooled(AtomicPool * pool,void * data)
	: _next(NULL)
	, _pool(pool)
	, _by_ev(NULL)
{
	_data = data;
	oflux_log_trace2("AC::alloc.g APD*this:%p _by_ev:%p %d\n"
		, this
		, _by_ev
		, __LINE__);
	init();
}

void
AtomicPooled::relinquish(bool)
{
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] APD::relinquish APD*this:%p\n"
		, oflux_self()
		, this);
	_pool->release(this);
}

const char *
AtomicPooled::atomic_class_str = "lockfree::Pooled";

bool
AtomicPooled::acquire_or_wait(EventBasePtr & ev,int t)
{
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
	_by_ev = get_EventBasePtr(ev);
	assert(_by_ev);
	__sync_synchronize();
	const char * flow_node_name = (_by_ev ? _by_ev->flow_node()->getName() : "<NULL>");
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AP::a_o_w ev %s ev*:%p  atom APD*this:%p "
		, oflux_self()
		, flow_node_name
		, _by_ev
		, this);
	bool acqed = _pool->waiters->acquire_or_wait(this); // try to acquire the resource
	assert(!acqed || _data);
	if(!acqed) {
		checked_recover_EventBasePtr(ev);
	}
	oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AP::a_o_w ev %s ev*:%p APD*this:%p res %d\n"
		, oflux_self()
		, flow_node_name
		, _by_ev
		, this
		, acqed);
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
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
	assert(_data);
	EventBase * evb = get_EventBasePtr(ev);
	assert(evb == _by_ev && "should release from the same event you acquired from"); 
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AP::rel   ev %s ev*:%p  APD*this:%p _data:%p\n"
		, oflux_self()
		, evb ? evb->flow_node()->getName() : "<NULL>"
		, evb
		, this
		, _data);
	AtomicPooled * rel_apb = 
		reinterpret_cast<AtomicPooled *>(_pool->waiters->release(this));
	if(rel_apb) { 
		assert(rel_apb != this);
		assert(rel_apb->_by_ev);
		assert(rel_apb->_data);
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AP::rel   ev %s ev*:%p APD*this:%p passed to %s rel_apb->ev:%p rsrc:%p\n"
			, oflux_self()
			, evb ? evb->flow_node()->getName() : "<NULL>"
			, evb
			, this
			, rel_apb->_by_ev->flow_node()->getName()
			, rel_apb->_by_ev
			, rel_apb->_data);
		assert(rel_apb->_by_ev);
		rel_ev_vec.push_back(EventBasePtr(rel_apb->_by_ev));
		oflux_log_trace2("AC::alloc.p rel_apb:%p %d\n"
			, rel_apb
			, __LINE__);
	} else {
		oflux_log_trace("[" PTHREAD_PRINTF_FORMAT "] AP::rel   ev %s ev*:%p  APD*this:%p rsrc nothing out\n"
			, oflux_self()
			, evb ? evb->flow_node()->getName() : "<NULL>"
			, evb
			, this);
	}
	oflux::lockfree::store_load_barrier();
#ifdef OFLUX_DEEP_LOGGING
	AtomicPool::dump(_pool);
#endif // OFLUX_DEEP_LOGGING
}

Allocator<AtomicPooled,DeferFree> AtomicPool::allocator; // (new allocator::MemoryPool<sizeof(AtomicPooled)>());

AtomicPool::AtomicPool()
	: waiters(new PoolEventList())
{
}

AtomicPool::~AtomicPool()
{
	oflux_log_trace2("~AtomicPool APD*this:%p\n",this);
	// reclaim items from the free list
	delete waiters;
}

void
AtomicPool::release(AtomicPooled *ap)
{
	oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AP::release APD*:%p\n",oflux_self(),ap);
	AtomicPool::allocator.put(ap);
}

const void *
AtomicPool::get(oflux::atomic::Atomic * & a_out,const void *)
{
	char _here;
	static const char * p_here = &_here;
	AtomicPooled * ap_out = NULL;
	if(!ap_out) {
		void * p = NULL;
		ap_out = AtomicPool::allocator.get(this,p);
		oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AP::get APD*:%p new\n",oflux_self(),ap_out);
	} else {
		oflux_log_trace2("[" PTHREAD_PRINTF_FORMAT "] AP::get APD*:%p old\n",oflux_self(),ap_out);
	}
	a_out = ap_out;
	return &p_here;
}

void
AtomicPooled::check()
{
	//assert(_by_ebh->resource_loc == &_resource_ebh);
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

