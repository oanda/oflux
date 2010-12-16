#include <cstdio>
#include "OFluxGenerate_freeguard.h"
#include "atomic/OFluxAtomicInit.h"
#include "OFluxRunTimeAbstract.h"

FList::~FList()
{ // count and report
	FLEl * h = head;
	FLEl * d = NULL;
	size_t count = 0;
	while(h) {
		d = h;
		++count;
		h = h->next;
		delete d;
	}
	printf("~FList counts %u elements\n",count);
}

void FList::insert(FLEl * el)
{
	FLEl * h = NULL;
	do {
		h = head;
	} while(!__sync_bool_compare_and_swap(&head,h,el));
	el->next = h;
}

const size_t max_b = 10000;

extern oflux::shared_ptr<oflux::RunTimeAbstract> theRT;


int 
S(const S_in *, S_out * out, S_atoms *)
{
	static size_t b = 0;
	out->b = b++;
	if(b == max_b) {
		sleep(5); // allow the NS events to drain
		theRT->soft_kill();
	}
	return 0;
}

int 
NS(const NS_in *in, NS_out *, NS_atoms * atoms)
{
	FList * & ptr = atoms->f();
	FLEl * el = new FLEl();
	el->id = in->b;
	ptr->insert(el);
	return 0;
}
	
void init(int, char * argv[])
{
	OFLUX_GUARD_POPULATER(Ga,GaPop);
	Ga_key gak;
	GaPop.insert(&gak,new FList());
}

void deinit()
{
	OFLUX_GUARD_CLEAN(Ga, FList)
}
