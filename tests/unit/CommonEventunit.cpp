#include "CommonEventunit.h"

namespace oflux {
 class RunTimeAbstractForShim; 
} // namespace oflux

oflux::RunTimeAbstractForShim *eminfo = NULL; // need this symbol due to doors


std::ostream & operator<<(std::ostream & os, const Empty &) 
{ os << "<empty>"; return os; }

std::ostream & operator<<(std::ostream & os, const Data1 &d) 
{ os << "<data1 " << d.x << " " << d.y << ">"; return os; }

std::ostream & operator<<(std::ostream & os, const Data2 &d) 
{ os << "<data2 " << d.y << ">"; return os; }

Data1 g_out1;

int f_source(const Empty *, Data1 *out, AtomsEmpty *)
{
        *out = g_out1;
	return 0;
}

Data2 g_out2;

int f_succ(const Data1 *, Data2 *out, AtomsEmpty *)
{
        *out = g_out2;
	return 0;
}

int f_next(const Data2 *, Empty *, AtomsEmpty *)
{
        return 0;
}


struct c_sourceDetail {
	typedef Empty In_;
	typedef Data1 Out_;
	typedef AtomsEmpty Atoms_;
	typedef int (*nfunctype)(const In_ *,Out_ *,Atoms_ *);
	static nfunctype nfunc;
};
c_sourceDetail::nfunctype c_sourceDetail::nfunc = &f_source;

oflux::CreateNodeFn OFluxCommonEventTests::c_source = oflux::create<c_sourceDetail>;

struct c_succDetail {
	typedef Data1 In_;
	typedef Data2 Out_;
	typedef AtomsEmpty Atoms_;
	typedef int (*nfunctype)(const In_ *,Out_ *,Atoms_ *);
	static nfunctype nfunc;
};
c_succDetail::nfunctype c_succDetail::nfunc = &f_succ;

oflux::CreateNodeFn OFluxCommonEventTests::c_succ = oflux::create<c_succDetail>;

struct c_nextDetail {
	typedef Data2 In_;
	typedef Empty Out_;
	typedef AtomsEmpty Atoms_;
	typedef int (*nfunctype)(const In_ *,Out_ *,Atoms_ *);
	static nfunctype nfunc;
};
c_nextDetail::nfunctype c_nextDetail::nfunc = &f_next;

oflux::CreateNodeFn OFluxCommonEventTests::c_next = oflux::create<c_nextDetail>;

const char * empty = "";

OFluxCommonEventTests::OFluxCommonEventTests()
	: n_source("source","c_source",c_source,NULL,false,true,false,false,empty,empty)
	, n_succ("source","c_source", c_succ,NULL,false,false,false,false,empty,empty)
	, n_next("source","c_source", c_succ,NULL,false,false,false,false,empty,empty)
{
}

