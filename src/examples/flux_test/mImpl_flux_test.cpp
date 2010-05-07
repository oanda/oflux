#include "OFluxGenerate_flux_test.h"
#include <iostream>
#include <unistd.h>
#include <time.h>

unsigned int nodeCount_One = 0;
unsigned int nodeCount_Two = 0;
unsigned int nodeCount_Three = 0;
unsigned int nodeCount_Four = 0;
unsigned int nodeCount_Five = 0;
unsigned int nodeCount_Select = 0;
unsigned int nodeCount_Send = 0;
unsigned int nodeCount_Read = 0;
unsigned int nodeCount_Route = 0;

int 
init(int argc, char** argv)
{
    return 0;
}

using namespace std;

int One(const One_in *in, One_out *out, One_atoms *)
{
#ifdef DO_OUTPUT
    cout << "One\n";
#endif
    ++nodeCount_One;
    return 0;
}


int Two(const Two_in *in, Two_out *out, Two_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Two\n";
#endif
    ++nodeCount_Two;
    return 0;
}

int Three(const Three_in *in, Three_out *out, Three_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Three\n";
#endif
    ++nodeCount_Three;
    return 0;
}

int Four(const Four_in *in, Four_out *out, Four_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Four\n";
#endif
    ++nodeCount_Four;
    return 0;
}

int Five(const Five_in *in, Five_out *out, Five_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Five\n";
#endif
    ++nodeCount_Five;
    return 0;
}

int Route(const Route_in *in, Route_out *out, Route_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Route\n";
#endif
    ++nodeCount_Route;
    return 0;
}

int Select(Select_in const* in, Select_out* out, Select_atoms*)
{
#ifdef DO_OUTPUT
    cout << "Select\n";
#endif
    ++nodeCount_Select;
    return 0;
}

int Read(const Read_in *in, Read_out *out, Read_atoms *)
{
#ifdef DO_OUTPUT
    cout << "Read\n";
#endif
    ++nodeCount_Read;
    return 0;
}

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
    static int count_down = 1000;

#ifdef DO_OUTPUT
    cout << "Send\n";
#endif
    ++count;
    ++nodeCount_Send;
    if(count_down-- < 0) {
        time_t ntime = ::time(0);
        if(ntime != last) {
            cout << "Send - " << count << " times per second\n";
            cout << "Total nodes per second : " << nodeCount() << endl;
	    count_down = count/2;
            reset_nodeCount();
            count = 0;
            last = ntime;
	} else {
		count_down = 100;
	}
    }

    
    return 0;
}
