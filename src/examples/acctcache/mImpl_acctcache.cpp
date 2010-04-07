#include <stdio.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include "OFluxGenerate_acctcache.h"

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
int on_account = 0;
int finish_visit = 0;

int Start(const Start_in *, Start_out * so, Start_atoms *)
{
	int c = 0;
	usleep(100);
	for(int i = 0; i < start_counter; i++) {
		for(int j = i; j < start_counter; j++) {
			if(c > on_account) {
				on_account = c;
				so->account = c;
				return 0;
			}
			c++;
		}
	}
	start_counter++;
	on_account = 0;
	so->account = c;
	usleep(10000);
	if(start_counter > 1000) { exit(0); }
	return (start_counter < 100 ? 0 : 1);
}

int Finish(const Finish_in * fi, Finish_out *, Finish_atoms * fa)
{
	if(fa->account_lock() == NULL) {
		fa->account_lock() = new Account(fi->account);
	}
	std::string fv = "";
	char buff[50];
	sprintf(buff,"%d",finish_visit++);
	fv += buff;
	fv += ",";
	fa->account_lock()->append(fv.c_str());
	fa->account_lock()->print();
	//usleep(100);
	return 0;
}


