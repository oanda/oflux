#include "CommonEventunit.h"

using namespace oflux;

class OFluxEventTests : public OFluxCommonEventTests {
public:
	OFluxEventTests() {}
        virtual ~OFluxEventTests() {}
	virtual void SetUp() {}
	virtual void TearDown() {}
	
};

TEST_F(OFluxEventTests,Execute1) {
        CreateNodeFn createfn_source = n_source.getCreateFn();
        CreateNodeFn createfn_succ = n_succ.getCreateFn();
        CreateNodeFn createfn_next = n_next.getCreateFn();
        EventBaseSharedPtr ev_source(
                (*createfn_source)(EventBase::no_event_shared,NULL,&n_source));
        EventBaseSharedPtr ev_succ(
                (*createfn_succ)(ev_source,NULL,&n_succ));
        EventBaseSharedPtr ev_next(
                (*createfn_next)(ev_succ,NULL,&n_next));
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
