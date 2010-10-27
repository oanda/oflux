#ifndef OFLUX_DOOR_H
#define OFLUX_DOOR_H

#ifdef HAS_DOORS_IPC
# include <door.h>
# include <string>
# include <sys/stat.h>
# include <fcntl.h>
# include "event/OFluxEventDoor.h"
# include "event/OFluxEventOperations.h"
# include "OFluxRunTimeAbstract.h"
# include "flow/OFluxFlow.h"

# define DOOR_RETURN door_return(NULL,0,NULL,0)
# define START_DOORS door_server_create(RunTime_start_door_thread)

namespace oflux {
namespace flow {
 class Node;
} //namespace flow

namespace doors {

class DoorException {
public:
        DoorException(const char * mesg)
                        : _mesg(mesg)
        {}
private:
        std::string _mesg;
};

template< typename T >
class CheckDoorDataIsPOD {
public:
	union CheckPOD { 
		int __dontcare;
		T t;
	};
};

int open(const char * door_filename);

template<typename T>
class ClientDoor {
	CheckDoorDataIsPOD<T> _pod_check;
public:

	ClientDoor(const char * door_filename)
		: _door_filename(door_filename)
		, _door_descriptor(-1)
	{}
	~ClientDoor()
	{}

	int send(const T * in)
	{
		if(_door_descriptor < -1) {
			_door_descriptor = ::open(_door_filename.c_str(), O_RDONLY);
		}
		door_arg_t arg =
			{ reinterpret_cast<char *>(const_cast<T *>(in)) // data_ptr
			, sizeof(T) // data_size
			, NULL // desc_ptr
			, 0 // num_desc
			, NULL // rbuf
			, 0 // rbuf_size
			};
		return door_call(_door_descriptor,&arg);
	}

private:
	std::string _door_filename;
	int _door_descriptor;
};

typedef void (*server_proc_t)(void *cookie, char *argp, size_t arg_size,
        door_desc_t *dp, size_t n_desc);

int create(const char * filename, server_proc_t server_proc, void * cookie);
void destroy(int & door, const char * filename);

struct ServerDoorCookie {
	ServerDoorCookie(
		  const char * door_filename
		, oflux::RunTimeAbstract * rt);

	flow::Node *         flow_node;
	oflux::RunTimeAbstract * runtime;
};

template<typename TDetail>
class ServerDoor : public ServerDoorCookie {
	CheckDoorDataIsPOD<typename TDetail::Out_> _pod_check;
public:
	ServerDoor(const char * door_filename, oflux::RunTimeAbstract * rt)
		: ServerDoorCookie(door_filename, rt)
		, _door_filename(door_filename)
		, _door_descriptor(create(door_filename,server_procedure,this))
	{
		if(_door_descriptor < 0) {
			std::string str("could not create door");
			str += door_filename;
			throw DoorException(str.c_str());
		}
	}
	virtual ~ServerDoor()
	{
		destroy(_door_descriptor,_door_filename);
		_door_descriptor= -1;
	}

	static void
	server_procedure(
		  void * cookie
		, char * argp
		, size_t sz
		, door_desc_t *
		, size_t )
	{
		ServerDoorCookie * sdc = 
			reinterpret_cast<ServerDoorCookie *>(cookie);
		assert(sz == sizeof(typename TDetail::Out_));
		flow::Node * flow_node = sdc->flow_node;
		EventBasePtr ev(new DoorEvent<TDetail>(
			  argp
			, sz
			, flow_node
			));
		std::vector<EventBasePtr> evs;
		event::successors_on_no_error(evs,ev);
		sdc->runtime->submitEvents(evs);
		door_return(NULL,0,NULL,0);
	}
private:
	const char * _door_filename;
	int _door_descriptor;
};

class ServerDoorsContainer { // make this a runtime member
public:
	ServerDoorsContainer(RunTimeAbstract * rt)
		: _rt(rt)
	{}

	~ServerDoorsContainer();
	int create_doors();
private:
	RunTimeAbstract * _rt;
	std::vector<ServerDoorCookie *> _created_doors;	
};

} // namespace doors

template< typename TDetail >
doors::ServerDoorCookie *
create_door(
	  const char * door_dir
	, const char * door_node_name
	, RunTimeAbstract * rt)
{
	std::string door_fullpath_filename = door_dir;
	door_fullpath_filename += "/";
	door_fullpath_filename += rt->flow()->name();
	door_fullpath_filename += "/";
	door_fullpath_filename += door_node_name;
	return new doors::ServerDoor<TDetail>(door_fullpath_filename.c_str(), rt);
}

} // namespace oflux

#else // ! HAS_DOORS_IPC
# define DOOR_RETURN  /*nuthin*/
# define START_DOORS  /*nuthin*/
struct door_info_t {};
namespace oflux {
namespace doors {

class ServerDoorCoookie {};

class ServerDoorsContainer {
public:
	int create_doors() { return 0; } // nothing to do
};
} // namespace doors

template< typename TDetail >
doors::ServerDoorCookie *
create_door(
          const char * door_dir
        , const char * door_node_name
        , RunTimeAbstract * rt)
{
#warning "no door IPC support"
	return NULL;
} 

} // namespace oflux
#endif  // HAS_DOORS_IPC

#endif // OFLUX_DOOR_H
