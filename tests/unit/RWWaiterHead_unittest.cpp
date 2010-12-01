
#include "lockfree/atomic/OFluxLFAtomicReadWrite.h"
#include <gtest/gtest.h>


using namespace oflux::lockfree::atomic;

class RWWaiterHeadTests : public testing::Test {
public:
	RWWaiterHeadTests() {}
	virtual ~RWWaiterHeadTests() {}
	virtual void SetUp() {}
	virtual void TearDown() {}
};

struct HS {
	EventBaseHolder * e;
	int rcount;
	bool mkd;
};

TEST_F(RWWaiterHeadTests,CASValues) {
	EventBaseHolder eh;

	HS hs[] =
	{ {&eh,0,false}
	, {&eh,1,false}
	, {&eh,-1,false}
	, {&eh,0,true}
	, {&eh,1,true}
	, {&eh,-1,true}
	, {NULL,0,false}
	, {NULL,1,false}
	, {NULL,-1,false}
	, {NULL,0,true}
	, {NULL,1,true}
	, {NULL,-1,true}
	};

	for(size_t i = 0; i < sizeof(hs)/sizeof(hs[0]); ++i) {
		RWWaiterHead head;
		RWWaiterHead head2(head);
		EXPECT_EQ(head2.head(),head.head());
		EXPECT_EQ(head2.rcount(),head.rcount());
		EXPECT_EQ(head2.mkd(),head.mkd());
		bool res_cas =
		head.compareAndSwap(head2,hs[i].e,hs[i].rcount,hs[i].mkd);
		EXPECT_TRUE(res_cas);
		EXPECT_EQ(head.head(),hs[i].e);
		EXPECT_EQ(head.rcount(),hs[i].rcount);
		EXPECT_EQ(head.mkd(),hs[i].mkd);
	}
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
