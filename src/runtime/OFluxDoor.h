#ifndef OFLUX_DOOR_H
#define OFLUX_DOOR_H

#ifdef HAS_DOORS_IPC
# include <door.h>
# include <string>
# include <errno.h>
# include <string.h>
# include <sys/stat.h>
# include <fcntl.h>
# include "event/OFluxEventDoor.h"
# include "event/OFluxEventOperations.h"
# include "OFluxRunTimeAbstract.h"
# include "flow/OFluxFlow.h"

# define OFLUX_DOOR_RETURN door_return(NULL,0,NULL,0)

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
		if(_door_descriptor < 0) {
			_door_descriptor = ::open(_door_filename.c_str(), O_RDONLY);
			if(_door_descriptor < 0) {
				oflux_log_error("::open() on door %s failed %s\n"
					, _door_filename.c_str()
					, strerror(errno));
			}
		}
		door_arg_t arg =
			{ reinterpret_cast<char *>(const_cast<T *>(in)) // data_ptr
			, sizeof(T) // data_size
			, NULL // desc_ptr
			, 0 // num_desc
			, NULL // rbuf
			, 0 // rbuf_size
			};
		int res = door_call(_door_descriptor,&arg);
		oflux_log_info("door_call returned %d on sz %u\n", res, arg.data_size);
		return res;
	}

private:
	std::string _door_filename;
	int _door_descriptor;
};

typedef void (*server_proc_t)(void *cookie, char *argp, size_t arg_size,
        door_desc_t *dp, size_t n_desc);

int create(const char * filename, server_proc_t server_proc, void * cookie);
void destroy(int & door, const char * filename);

struct ServerDoorCookie { // this is the base-level POD struct
	ServerDoorCookie(
		  const char * door_filename
		, oflux::RunTimeAbstract * rt);

	flow::Node *         flow_node;
	oflux::RunTimeAbstract * runtime;
};

class ServerDoorCookieVirtualDestructor : public ServerDoorCookie {
public:
	ServerDoorCookieVirtualDestructor(
		  const char * door_filename
		, oflux::RunTimeAbstract * rt)
		: ServerDoorCookie(door_filename,rt)
		{}
	virtual ~ServerDoorCookieVirtualDestructor() {}
};

template<typename TDetail>
class ServerDoor : public ServerDoorCookieVirtualDestructor {
	CheckDoorDataIsPOD<typename TDetail::Out_> _pod_check;
public:
	ServerDoor(const char * door_filename, oflux::RunTimeAbstract * rt)
		: ServerDoorCookieVirtualDestructor(door_filename, rt)
		, _door_filename(door_filename)
		, _door_descriptor(create(door_filename,server_procedure
			,reinterpret_cast<ServerDoorCookie *>(this)))
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
		oflux_log_info("server_procedure called on sz %u\n", sz);
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

typedef void (*RunTime_start_door_function)(door_info_t *);

class ServerDoorsContainer { // make this a runtime member
public:
	ServerDoorsContainer(RunTimeAbstract * rt)
		: _rt(rt)
	{}

	~ServerDoorsContainer();
	int create_doors(RunTime_start_door_function sdfc);
private:
	RunTimeAbstract * _rt;
	std::vector<ServerDoorCookieVirtualDestructor *> _created_doors;	
};

} // namespace doors

template< typename TDetail >
doors::ServerDoorCookieVirtualDestructor *
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
	doors::ServerDoorCookieVirtualDestructor * sc = 
		new doors::ServerDoor<TDetail>(door_fullpath_filename.c_str(), rt);
	return sc;
}

} // namespace oflux

#else // ! HAS_DOORS_IPC
# define OFLUX_DOOR_RETURN  /*nuthin*/
struct door_info_t {};
namespace oflux {
namespace doors {

class ServerDoorCookie {};
class ServerDoorCookieVirtualDestructor : public ServerDoorCookie {};

class ServerDoorsContainer {
public:
	int create_doors() { return 0; } // nothing to do
};
} // namespace doors

template< typename TDetail >
doors::ServerDoorCookieVirtualDestructor *
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
