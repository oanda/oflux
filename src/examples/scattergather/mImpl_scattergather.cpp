#include "OFluxGenerate_scattergather.h"
#include "OFlux.h"
#include "OFluxRunTimeAbstract.h"

// CPU eater
int 
fib(int n) 
{
	return ( n <= 1 ? 1 : fib(n-1) + fib(n-2) );
}

bool
isLast(const oflux::Scatter<GatherObj> & so)
{
	std::cout << "isLast " << so->count() << std::endl;
	return so->count() == 1;
}

extern boost::shared_ptr<oflux::RunTimeAbstract> theRT;

int
Foo(      const Foo_in * in
	, Foo_out * out
	, Foo_atoms * atoms)
{
	std::cout << " Foo called\n";
	int res = 0;
	size_t sz = 30;
	oflux::PushTool<Foo_out> ptool(out);
	oflux::Scatter<GatherObj> sctr(new GatherObj(sz)); 
	for(size_t i = 0; i < sz; ++i) {
		ptool->so.clone(sctr);
		ptool->d = i;
		std::cout << "Foo: push " << ptool->so->count() << "\n";
		ptool.next();
	}
	return res;
}

int
Bar(      const Bar_in * in
	, Bar_out * out
	, Bar_atoms * atoms)
{
	int res = 0;
	out->so = in->so; // pass it through (don't forget)
	out->so->put(in->d,fib(in->d));
	return res;
}

int
Collect(  const Collect_in * in
	, Collect_out * out
	, Collect_atoms * atoms)
{
	std::cout << " Collect called " << std::endl;
	in->so->dump();
	theRT->soft_kill();
	return 0;
}
