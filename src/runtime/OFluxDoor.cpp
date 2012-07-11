/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAS_DOORS_IPC
# include "OFluxDoor.h"
# include "OFluxLogging.h"
# include <unistd.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <cassert>
# include <errno.h>
# include <string.h>
# ifdef __linux__
#  include <namefs.h>
# endif // __linux__

namespace oflux {
namespace doors {

ServerDoorCookie::ServerDoorCookie(
	  const char * door_filename
	, oflux::RunTimeAbstract * rt)
	: flow_node(NULL)
	, runtime(rt)
{
	flow::Flow * flow = rt->flow();
	assert(flow);
	std::string node_name = door_filename;
	size_t slashpos = node_name.find_last_of('/');
	if(slashpos != std::string::npos) {
		node_name.replace(0,slashpos+1,"");
	}
	flow_node = flow->get<oflux::flow::Node>(node_name);
	assert(flow_node && flow_node->getIsDoor());
}


int
open(const char * door_filename)
{
	int fd = -1;
	int retries = 1001;
	while(1) {
		if(--retries < 0) {
			oflux_log_error("oflux::doors::open() exceeded retry count on %s\n", door_filename);
		}
		fd = ::open(door_filename, O_RDONLY);
		if(fd < 0) {
			if(errno != ENOENT) {
				oflux_log_error("oflux::doors::open() failed %s\n"
					, strerror(errno));
			}
		}
		if(fd < 0) {
			std::string str("could not open a door ");
			str += door_filename;
			throw DoorException(str.c_str());
		} else {
			struct stat buf;
			fstat(fd,&buf);
			if(!S_ISREG(buf.st_mode)) {
				break;
			}
			close(fd);
		}
		usleep(1000);
	}
	return fd;
}

int
create(const char * filename, server_proc_t server_proc, void * cookie)
{
	int fd;
	int door;
	door = ::door_create(server_proc,cookie,0);
	if(door > 0) {
		std::string dir(filename);
		size_t last_slash = dir.find_last_of('/');
		if(last_slash != std::string::npos) {
			dir = dir.substr(0,last_slash);
			struct stat s;
			if(::stat(dir.c_str(),&s) < 0) {
				::mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			}
		}
		fd = ::creat(filename,0600);
		::close(fd);
		if(fd > 0) {
			::fattach(door,filename);
		} else {
			door = -1;
		}
	}
	return door;
}

void
destroy(int & door, const char * filename)
{
	if(door >= 0) {
		::close(door);
		::fdetach(filename);
		::unlink(filename);
		door = -1;
	}
}

ServerDoorsContainer::~ServerDoorsContainer()
{
	for(size_t i = 0; i < _created_doors.size(); ++i) {
		delete _created_doors[i];
	}
	_created_doors.clear();
}

int
ServerDoorsContainer::create_doors(RunTime_start_door_function sdfc)
{
	assert(_rt);
	flow::Flow * flow = _rt->flow();
	assert(flow);
	std::vector<flow::Node *> & doors = flow->doors();
	if(!doors.empty()) {
		door_server_create(sdfc);
	}
	for(size_t i = 0; i < doors.size(); ++i) {
		CreateDoorFn create_door_fn = doors[i]->getCreateDoorFn();
		ServerDoorCookieVirtualDestructor * cookie = 
			(*create_door_fn)(
				  _rt->config().doors_dir
				, doors[i]->getName()
				, _rt);
		_created_doors.push_back(cookie);
	}
	return doors.size();
}

} // namespace doors
} // namespace oflux
#endif // HAS_DOORS_IPC
