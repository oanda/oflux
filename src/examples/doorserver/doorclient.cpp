#include "OFluxGenerate_doorserver.h"
#include "OFluxDoor.h"

int
main(int argc, char * argv[])
{
	oflux::doors::ClientDoor<S_out> door("/tmp/doorserver/S");
	S_out s;
	int sz = argc ? atoi(argv[1]) : 1;
	for(int i=0; i < sz; ++i) {
		s.a = i;
		door.send(&s);
	}
	return 0;
}
