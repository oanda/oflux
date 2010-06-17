#ifndef MIMPL_TIMER_INTERNAL_H
#define MIMPL_TIMER_INTERNAL_H

#include <map>
#include <pthread.h>

namespace Timer {

template<typename T>
class LockFreeList {
public:
	class Element {
	public:
		Element(T & at)
			: t(at)
			, _next(NULL)
		{}

		T t;

		Element * next() { return _next; }
		void next(Element *e) { _next = e; }
	private:
		Element * _next;
	};

	LockFreeList()
		: _head(NULL) // sentinel
	{}

	~LockFreeList()
	{
		Element * h = NULL;
		do {
			h = _head;
		} while(!__sync_bool_compare_and_swap(&_head,h,NULL));
		Element * lh = h;
		while(h) {
			lh = h;
			h = h->next();
			delete lh;
		}
	}

	template<typename F>
	void fold(void (*foldFun)(F &,T &), F & fixed_point)
	{
		Element * h = _head;
		while(h) {
			(*foldFun)(fixed_point,h->t);
			h = h->next();
		}
	}

	void add(T & t)
	{
		Element * e = new Element(t);
		Element * h = NULL;
		do {
			h = _head;
			e->next(h);
		} while(!__sync_bool_compare_and_swap(&_head,h,e));
	}

	void swap(LockFreeList & lfl_local_not_mt)
	{
		// assumption: lfl_local_not_mt should not get any asynch update
		// (so it cannot change during this call)
		Element * h = NULL;
		h = lfl_local_not_mt._head;
		Element * h_us = NULL;
		do {
			h_us = _head;
		} while(!__sync_bool_compare_and_swap(&_head,h_us,h));
		lfl_local_not_mt._head = h_us;
	}
		
private:
	Element * _head;
};

class State {
public:
	State(int sig=SIGALRM)
		: until_time(0)
		, _thread_id(0)
		, _signal(sig)
	{}
	
	void asynch_cancel(size_t id) // can call from any thread at anytime
	{ _cancel_list.add(id); }
	void asynch_add(EventBase * eb)
	{ _new_list.add(eb); }
	int signal() 
	{ return _thread_id ? pthread_kill(_thread_id,_signal) : 0; }

	EventBase * remove(size_t id); // not MT
	bool add(EventBase * eb);  // not MT
	EventBase * find(size_t id); // not MT

	// return the next time_t when something will happen (side-effect in until_time)
	time_t synch_update_internals(
		  time_t present_time
		, std::vector<EventBase *> & expired
		, std::vector<EventBase *> & cancelled);

	class ClaimThreadIdRAII {
	public:
		ClaimThreadIdRAII(State & s)
			: _s(s)
		{ _s._thread_id = oflux_self(); }
		~ClaimThreadIdRAII()
		{ _s._thread_id = 0; }
	private:
		State & _s;
	};

	time_t until_time;
protected:
	volatile oflux::oflux_thread_t _thread_id;
private:
	int _signal;
	LockFreeList<size_t> _cancel_list;
	LockFreeList<EventBase *> _new_list;
	std::map<size_t,EventBase *> _map_by_id;
	std::multimap<time_t,EventBase *> _map_by_time;
};


} // namespace Timer

#endif 
