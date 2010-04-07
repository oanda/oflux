#include "OFluxGenerate_splaytest2.h"
#include <iostream>

int gCount = 0;

int S(const S_in *, S_out * out, S_atoms *)
{
	sleep(5);
	oflux::PushTool<S_out> pt(out);
	pt->foo = FooPtr(new Foo(gCount++));
	pt.next();
	pt->foo = FooPtr(new Foo(gCount++));
	pt.next();
	pt->foo = FooPtr(new Foo(gCount++));
	pt.next();
	return 0;
}

int NS(const NS_in *in, NS_out * out, NS_atoms *)
{
    std::cout << "NS got foo " << in->foo->foo_
              << " addr " << in->foo.get()
              << " count " << in->foo.use_count() << std::endl;
	return 0;
}
