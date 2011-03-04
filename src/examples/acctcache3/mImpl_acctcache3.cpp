#include <stdio.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "OFluxGenerate_acctcache3.h"
#include "OFluxRunTimeBase.h"

class Account {
public:
	Account(int anaccount) : account(anaccount) {}
	void append(const char * cstr) { str += cstr; }
	void print() { printf("%d %s.\n",account,str.c_str()); }
private:
	int account;
	std::string str;
};

int start_counter = 0;

extern oflux::shared_ptr<oflux::RunTimeAbstract> theRT;

void handlesigusr1(int)
{
    signal(SIGUSR1,handlesigusr1);
    if(theRT.get()) theRT->log_snapshot();
}


bool 
register_signal_handler()
{
	signal(SIGUSR1,handlesigusr1);
	return true;
}

int 
Start(const Start_in *, Start_out * so, Start_atoms *)
{
	static bool foo __attribute__((unused)) = register_signal_handler();
	so->account = ++start_counter;
	if(start_counter >= 1000) {
		usleep(1);
		start_counter = 0;
	}
	return 0;
}

int 
Finish(const Finish_in * fi, Finish_out *, Finish_atoms * fa)
{
	if(fa->account_lock() == NULL) {
		fa->account_lock() = new Account(fi->account);
	}
	//usleep(100);
	return 0;
}


