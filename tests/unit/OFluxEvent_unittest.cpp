#include "OFluxEvent.h"
#include "OFluxFlow.h"
#include <gtest/gtest.h>

using namespace oflux;


class AtomsEmpty {
public:
        void fill(oflux::AtomicsHolder * ah) {}
};

class Empty : public BaseOutputStruct<Empty> { 
public:
        bool operator==(const Empty &) const { return true; }
typedef Empty base_type;
};

std::ostream & operator<<(std::ostream & os, const Empty &) 
{ os << "<empty>"; return os; }

class Data1 : public BaseOutputStruct<Data1> {
public:
  typedef Data1 base_type;
  int x;
  long y;
  bool operator==(const Data1 &o) const{ return (o.x==x) && (o.y==y); }
};

std::ostream & operator<<(std::ostream & os, const Data1 &d) 
{ os << "<data1 " << d.x << " " << d.y << ">"; return os; }

class Data2 : public BaseOutputStruct<Data2> {
public:
  typedef Data2 base_type;
  int y;
  bool operator==(const Data1 &o) const { return (o.y==y); }
};
std::ostream & operator<<(std::ostream & os, const Data2 &d) 
{ os << "<data2 " << d.y << ">"; return os; }

namespace oflux {
template<>
inline Empty * convert<Empty>(Empty::base_type *a) { return a; }
template<>
inline const Empty * const_convert<Empty>(const Empty::base_type *a) { return a; }
template<>
inline Data1 * convert<Data1>(Data1::base_type *a) { return a; }
template<>
inline const Data1 * const_convert<Data1>(const Data1::base_type *a) { return a; }
template<>
inline Data2 * convert<Data2>(Data2::base_type *a) { return a; }
template<>
inline const Data2 * const_convert<Data2>(const Data2::base_type *a) { return a; }
} // namespace oflux

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

class OFluxEventTests : public testing::Test {
public:
	OFluxEventTests();
        virtual ~OFluxEventTests() {}
	virtual void SetUp() {}
	virtual void TearDown() {}
	
	static CreateNodeFn c_source;
	static CreateNodeFn c_succ;
	static CreateNodeFn c_next;

//private:
	flow::Node n_source;
	flow::Node n_succ;
	flow::Node n_next;
};

CreateNodeFn OFluxEventTests::c_source = 
        create<Empty,Data1,AtomsEmpty,&f_source>;
CreateNodeFn OFluxEventTests::c_succ = 
        create<Data1,Data2,AtomsEmpty,&f_succ>;
CreateNodeFn OFluxEventTests::c_next = 
        create<Data2,Empty,AtomsEmpty,&f_next>;

const char * empty = "";

OFluxEventTests::OFluxEventTests()
	: n_source("source", "c_source",c_source,false,true,false,empty,empty)
	, n_succ("source", "c_source",c_succ,false,false,false,empty,empty)
	, n_next("source", "c_source",c_succ,false,false,false,empty,empty)
{
}

TEST_F(OFluxEventTests,Execute1) {
        CreateNodeFn createfn_source = n_source.getCreateFn();
        CreateNodeFn createfn_succ = n_succ.getCreateFn();
        CreateNodeFn createfn_next = n_next.getCreateFn();
        boost::shared_ptr<EventBase> ev_source =
                (*createfn_source)(EventBase::no_event,NULL,&n_source);
        boost::shared_ptr<EventBase> ev_succ =
                (*createfn_succ)(ev_source,NULL,&n_succ);
        boost::shared_ptr<EventBase> ev_next =
                (*createfn_next)(ev_succ,NULL,&n_next);
        g_out1.x = 999;
        g_out1.y = -999;
        int r_source = ev_source->execute();
        EXPECT_EQ(0,r_source) << "source returns 0";
        {
                OutputWalker ev_ow = ev_source->output_type();
                void * ev_output = ev_ow.next();
                Data1 * d1_out = reinterpret_cast<Data1 *>(ev_output);
                EXPECT_EQ(g_out1,*d1_out) << "source returnval check";
        }
        int r_succ = ev_succ->execute();
        EXPECT_EQ(0,r_succ) << "succ returnval check";
        int r_next = ev_next->execute();
        EXPECT_EQ(0,r_next) << "next returnval check";
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
