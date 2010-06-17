#include "OFluxGenerate_timer1_Timer.h"
#include "mImpl_Timer_internal.h"
#include <ctime>
#include <unistd.h>
#include <map>

namespace Timer {

State *
factory_State(int sig)
{
	return new State(sig);
}

size_t
EventBase::unique_id_generator()
{
	static volatile size_t id = 1;
	size_t mine = __sync_fetch_and_add(&id,1);
	return mine;
}

namespace update_folds {

struct StateAndRemoves {
	StateAndRemoves(State & s, std::vector<EventBase *> & rs)
		: state(s)
		, removes(rs)
	{}

	State & state;
	std::vector<EventBase *> & removes;
};

void remove(StateAndRemoves & sar, size_t & id)
{
	EventBase * eb = sar.state.remove(id);
	if(eb) {
		sar.removes.push_back(eb);
	}
}

void add(State & s, EventBase * & eb)
{
	s.add(eb);
}

} // namespace update

time_t 
State::synch_update_internals(
	  time_t present_time
	, std::vector<EventBase *> & expired
	, std::vector<EventBase *> & cancelled)
{
	{ // handle the new Events added
		LockFreeList<EventBase *> toadd;
		_new_list.swap(toadd);
		toadd.fold<State>(&update_folds::add,*this);
	}
	
	{ // handle the cancels
		LockFreeList<size_t> tocancel;
		_cancel_list.swap(tocancel);
		update_folds::StateAndRemoves sar(*this,cancelled);
		tocancel.fold<update_folds::StateAndRemoves>(&update_folds::remove,sar);
	}

	// find expired
	std::multimap<time_t,EventBase *>::iterator mmitr = _map_by_time.begin();
	while(mmitr != _map_by_time.end() && (*mmitr).first <= present_time) {
		expired.push_back((*mmitr).second);
		++mmitr;
	}
	// remove expired from internal structures
	std::vector<EventBase *>::iterator vitr = expired.begin();
	while(vitr != expired.end()) {
		remove((*vitr)->id());
		++vitr;
	}
	return until_time;
}

EventBase *
State::remove(size_t id)
{
	EventBase * eb = NULL;
	std::map<size_t,EventBase *>::iterator itr =
		_map_by_id.find(id);
	if(itr != _map_by_id.end()) {
		eb = (*itr).second;
		std::multimap<time_t,EventBase *>::iterator mitr =
			_map_by_time.lower_bound(eb->expiry());
		while(mitr != _map_by_time.end() 
				&& (*mitr).second != eb
				&& (*mitr).second->expiry() <= eb->expiry()) {
			++mitr;
		}
		_map_by_id.erase(itr);
		if((*mitr).second == eb) {
			_map_by_time.erase(mitr);
		}
	}
	return eb;
}

bool
State::add(EventBase *eb)
{
	std::pair<size_t, EventBase *> pr(eb->id(),eb);
	bool res = _map_by_id.insert(pr).second;
	if(res) {
		std::pair<time_t,EventBase *> pr2(eb->expiry(),eb);
		_map_by_time.insert(pr2);
	}
	return res;
}

EventBase *
State::find(size_t id)
{
	std::map<size_t, EventBase *>::iterator itr = 
		_map_by_id.find(id);
	return (itr == _map_by_id.end() ? NULL : (*itr).second);
}

int
Expire_splay(
	  const Expire_splay_in * in
	, Expire_splay_out * out
	, Expire_splay_atoms * atoms)
{
	State * & state = atoms->state();
	State::ClaimThreadIdRAII claim(*state);
	std::vector<EventBase *> expired;
	std::vector<EventBase *> cancelled;
	time_t present_time = time(NULL);
	if(state->until_time > present_time) {
		sleep(state->until_time - present_time); // not terribly accurate
	}
	state->synch_update_internals(
		  present_time
		, expired
		, cancelled);
	oflux::PushTool<Expire_splay_out> ptool(out);
	std::vector<EventBase *>::iterator itr;
	int push_count = 0;
	for(itr = expired.begin(); itr != expired.end(); ++itr) {
		ptool->timer.reset(*itr);
		ptool->is_cancelled = false;
		ptool.next();
		++push_count;
	}
	for(itr = cancelled.begin(); itr != cancelled.end(); ++itr) {
		ptool->timer.reset(*itr);
		ptool->is_cancelled = true;
		ptool.next();
		++push_count;
	}
	return (push_count > 0 ? 0 : -1);
}

} // namespace Timer
