#include "OFluxGenerate.h"
#include <iostream>
#include <sys/types.h>
#include <time.h>

using namespace std;

inline int nodeCount()
{
	return nodeCount_One
		+ nodeCount_Two
		+ nodeCount_Three
		+ nodeCount_Four
		+ nodeCount_Five
		+ nodeCount_Select
		+ nodeCount_Read
		+ nodeCount_Route
		+ nodeCount_Send;
}

void reset_nodeCount()
{
	nodeCount_One = 0;
	nodeCount_Two = 0;
	nodeCount_Three = 0;
	nodeCount_Four = 0;
	nodeCount_Five = 0;
	nodeCount_Select = 0;
	nodeCount_Read = 0;
	nodeCount_Route = 0;
	nodeCount_Send = 0;
}

int Send(const Send_in * in, Send_out * out, Send_atoms *)
{
    static unsigned int count = 0;
    static time_t last = ::time(0);

#ifdef DO_OUTPUT
    cout << "Send\n";
#endif
    ++count;
    ++nodeCount_Send;
    if(::time(0) != last) {
        cout << "Send - " << count << " times per second\n";
        cout << "Total nodes per second : " << nodeCount() << endl;
        reset_nodeCount();
        count = 0;
        last = ::time(0);
    }
    
    return 0;
}
