#include "CommonEventunit.h"

using namespace oflux;

class OFluxAtomicTests : public OFluxCommonEventTests {
public:
	OFluxAtomicTests(atomic::Atomic *);
        virtual ~OFluxAtomicTests() {}
	virtual void SetUp() {}
	virtual void TearDown() {}

        void checkRelease(
		  EventBasePtr & ev
		, std::vector<EventBasePtr> rel_ev);
        void checkRelease(
		  EventBasePtr & ev
		, EventBasePtr & rel_ev);
        void checkRelease(
		  EventBasePtr & ev);

        CreateNodeFn createfn_source;
        CreateNodeFn createfn_succ;
        CreateNodeFn createfn_next;

        atomic::Atomic * _atomic_ptr;
};
void
OFluxAtomicTests::checkRelease(EventBasePtr & ev)
{
	std::vector<EventBasePtr> vec; // empty
	checkRelease(ev,vec);
}

void
OFluxAtomicTests::checkRelease(
                  EventBasePtr & ev
		, EventBasePtr & rel_ev)
{
	std::vector<EventBasePtr> vec;
	vec.push_back(rel_ev);
	checkRelease(ev,vec);
}

void 
OFluxAtomicTests::checkRelease(
		  EventBasePtr & ev
		, std::vector<EventBasePtr> rel_ev)
{
        std::vector<EventBasePtr> test_rel_ev;
        _atomic_ptr->release(test_rel_ev,ev);
	EXPECT_EQ(test_rel_ev.size(),rel_ev.size())
		<< " number of released events";
	for(size_t i = 0 ; i < std::min(test_rel_ev.size(),rel_ev.size()); ++i) {
		EXPECT_EQ(get_EventBasePtr(test_rel_ev[i])
				,get_EventBasePtr(rel_ev[i]))
			<< " released event " << i << " comparison";
	}
}


OFluxAtomicTests::OFluxAtomicTests(atomic::Atomic * aptr)
	: _atomic_ptr(aptr)
{
        createfn_source = n_source.getCreateFn();
        createfn_succ = n_succ.getCreateFn();
        createfn_next = n_next.getCreateFn();
}

class OFluxAtomicExclusiveTests : public OFluxAtomicTests {
public:
        OFluxAtomicExclusiveTests() 
                : OFluxAtomicTests(&atom)
                , data_(0)
                , atom(&data_)
                {}

        int data_;
        atomic::AtomicExclusive atom;
        static const int excl;
};

const int OFluxAtomicExclusiveTests::excl = atomic::AtomicExclusive::Exclusive;

TEST_F(OFluxAtomicExclusiveTests,ThreeEventsDrain) {
        EventBaseSharedPtr ev_source_shared (
                (*createfn_source)(EventBase::no_event_shared,NULL,&n_source));
        EventBasePtr ev_source = get_EventBaseSharedPtr(ev_source_shared); 
        EventBaseSharedPtr ev_succ_shared (
                (*createfn_succ)(ev_source_shared,NULL,&n_succ));
        EventBasePtr ev_succ = get_EventBaseSharedPtr(ev_succ_shared); 
        EventBaseSharedPtr ev_next_shared (
                (*createfn_next)(ev_succ_shared,NULL,&n_next));
        EventBasePtr ev_next = get_EventBaseSharedPtr(ev_next_shared); 

        EXPECT_TRUE (atom.acquire_or_wait(ev_source,excl));
        EXPECT_TRUE (atom.held());
        EXPECT_FALSE(atom.acquire_or_wait(ev_succ,excl));
        EXPECT_EQ(atom.waiter_count(),1);
        EXPECT_FALSE(atom.acquire_or_wait(ev_next,excl));
        EXPECT_EQ(atom.waiter_count(),2);
        checkRelease(ev_source,ev_succ);
        EXPECT_TRUE (atom.held());
        EXPECT_EQ(atom.waiter_count(),1);
        checkRelease(ev_succ,ev_next);
        EXPECT_TRUE (atom.held());
        checkRelease(ev_next);
        EXPECT_FALSE(atom.held());
        EXPECT_TRUE (atom.acquire_or_wait(ev_next,excl));
        EXPECT_FALSE(atom.acquire_or_wait(ev_source,excl));
        EXPECT_EQ(atom.waiter_count(),1);
        checkRelease(ev_next,ev_source);
        EXPECT_EQ(atom.waiter_count(),0);
        checkRelease(ev_source);
        EXPECT_EQ(atom.waiter_count(),0);
}

class OFluxAtomicReadWriteTests : public OFluxAtomicTests {
public:
        OFluxAtomicReadWriteTests() 
                : OFluxAtomicTests(&atom)
                , data_(0)
                , atom(&data_)
                {}

        int data_;
        atomic::AtomicReadWrite atom;
        static const int read;
        static const int write;
};

const int OFluxAtomicReadWriteTests::read = atomic::AtomicReadWrite::Read;
const int OFluxAtomicReadWriteTests::write = atomic::AtomicReadWrite::Write;

TEST_F(OFluxAtomicReadWriteTests,TwoReadThenOneWrite1) {
        EventBaseSharedPtr ev_any_reader1_shared (
                (*createfn_next)(EventBase::no_event_shared,NULL,&n_next));
        EventBasePtr ev_any_reader1 = get_EventBaseSharedPtr(ev_any_reader1_shared); 
        EventBaseSharedPtr ev_any_reader2_shared (
                (*createfn_next)(EventBase::no_event_shared,NULL,&n_next));
        EventBasePtr ev_any_reader2 = get_EventBaseSharedPtr(ev_any_reader2_shared); 
        EventBaseSharedPtr ev_writer_shared (
                (*createfn_source)(EventBase::no_event_shared,NULL,&n_source));
        EventBasePtr ev_writer = get_EventBaseSharedPtr(ev_writer_shared); 

        // 2 readers added:
        EXPECT_EQ(0,atom.held());
        EXPECT_TRUE(atom.acquire_or_wait(ev_any_reader1,read));
        EXPECT_EQ(1,atom.held());
        EXPECT_TRUE(atom.acquire_or_wait(ev_any_reader2,read));
        EXPECT_EQ(2,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        // 1 writer queued:
        EXPECT_FALSE(atom.acquire_or_wait(ev_writer,write));
        EXPECT_EQ(2,atom.held());
        EXPECT_EQ(atom.waiter_count(),1);
        // release 2 readers
        checkRelease(ev_any_reader1);
        checkRelease(ev_any_reader2, ev_writer);
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        checkRelease(ev_writer);
        EXPECT_EQ(atom.waiter_count(),0);
        EXPECT_EQ(0,atom.held());
        // writer acquires
        EXPECT_TRUE (atom.acquire_or_wait(ev_writer,write));
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        EXPECT_FALSE(atom.acquire_or_wait(ev_any_reader1,read));
        checkRelease(ev_writer,ev_any_reader1);
        EXPECT_EQ(1,atom.held());
}

TEST_F(OFluxAtomicReadWriteTests,ReadWriteRead1) {
        EventBaseSharedPtr ev_reader1_shared (
                (*createfn_next)(EventBase::no_event_shared,NULL,&n_next));
        EventBasePtr ev_reader1 = get_EventBaseSharedPtr(ev_reader1_shared); 
        EventBaseSharedPtr ev_reader2_shared (
                (*createfn_next)(EventBase::no_event_shared,NULL,&n_next));
        EventBasePtr ev_reader2 = get_EventBaseSharedPtr(ev_reader2_shared); 
        EventBaseSharedPtr ev_writer_shared (
                (*createfn_source)(EventBase::no_event_shared,NULL,&n_source));
        EventBasePtr ev_writer = get_EventBaseSharedPtr(ev_writer_shared); 

        // 1 readers added:
        EXPECT_EQ(0,atom.held());
        EXPECT_TRUE(atom.acquire_or_wait(ev_reader1,read));
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        // 1 writer must wait
        EXPECT_FALSE(atom.acquire_or_wait(ev_writer,write));
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),1);
        // 1 reader must wait
        EXPECT_FALSE(atom.acquire_or_wait(ev_reader2,read))
                << "should fail since one writer waits";
        EXPECT_EQ(atom.waiter_count(),2);
        EXPECT_EQ(1,atom.held());
        // writer succeeds
        checkRelease(ev_reader1,ev_writer);
        EXPECT_EQ(atom.waiter_count(),1);
        EXPECT_EQ(1,atom.held());
        // writer releases
        checkRelease(ev_writer,ev_reader2);
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        // second reader acquires
        EXPECT_EQ(atom.waiter_count(),0);
        checkRelease(ev_reader2);
        EXPECT_EQ(0,atom.held());
}

TEST_F(OFluxAtomicReadWriteTests,WriteReadRead1) {
        EventBaseSharedPtr ev_reader1_shared (
                (*createfn_next)(EventBase::no_event_shared,NULL,&n_next));
        EventBasePtr ev_reader1 = get_EventBaseSharedPtr(ev_reader1_shared); 
        EventBaseSharedPtr ev_reader2_shared (
                (*createfn_source)(EventBase::no_event_shared,NULL,&n_source));
        EventBasePtr ev_reader2 = get_EventBaseSharedPtr(ev_reader2_shared); 
        EventBaseSharedPtr ev_writer_shared (
                (*createfn_source)(EventBase::no_event_shared,NULL,&n_source));
        EventBasePtr ev_writer = get_EventBaseSharedPtr(ev_writer_shared); 

        // 1 writer added:
        EXPECT_EQ(0,atom.held());
        EXPECT_TRUE(atom.acquire_or_wait(ev_writer,write));
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        // 1 reader must wait
        EXPECT_FALSE(atom.acquire_or_wait(ev_reader1,read));
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),1);
        // 2nd reader must wait
        EXPECT_FALSE(atom.acquire_or_wait(ev_reader2,read));
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),2);
        // writer releases
        std::vector<EventBasePtr> rel_ev;
	rel_ev.push_back(ev_reader1);
	rel_ev.push_back(ev_reader2);
        checkRelease(ev_writer,rel_ev);
        EXPECT_EQ(atom.waiter_count(),0);
        EXPECT_EQ(2,atom.held());
        // both readers release
        checkRelease(ev_reader1);
        checkRelease(ev_reader2);
        EXPECT_EQ(0,atom.held());
}

class OFluxAtomicPooledTests : public OFluxAtomicTests {
public:
        OFluxAtomicPooledTests() 
                : OFluxAtomicTests(NULL)
                , data1_(1)
                , data2_(2)
                , atom1(NULL)
                , atom2(NULL)
        { 
		// make n_next grab a pool guard
		static oflux::flow::Guard g(&pool,"testpool", false);
		static oflux::flow::GuardReference * gr =
			new oflux::flow::GuardReference(&g,pl,false);
		n_next.add(gr);
                // mimic populator
                pool.get(atom1,NULL);
                void ** d1 = atom1->data();
                *d1 = &data1_;
                pool.get(atom2,NULL);
                void ** d2 = atom2->data();
                *d2 = &data2_;
                std::vector<EventBasePtr> vec;
		EventBasePtr no_event;
                atom1->release(vec,no_event);
                atom2->release(vec,no_event);
                if(vec.size()) { throw -1; }
        }
        virtual ~OFluxAtomicPooledTests()
        {
                //delete atom1;
                //delete atom2;
        }

        int data1_;
        int data2_;
        atomic::Atomic *atom1;
        atomic::Atomic *atom2;
        atomic::AtomicPool pool;
        static const int pl;
};

class AcquirePoolItem {
public:
        AcquirePoolItem(
                  EventBasePtr & ev
                , int expect_data)
        {
                //pool.get(a,NULL);
                //EXPECT_TRUE(NULL != a);
		oflux::atomic::AtomicsHolder & atomics = ev->atomics();
		oflux::atomic::HeldAtomic * ha = atomics.get(0);
                was_held = ha->haveit();
                have = atomics.acquire_all_or_wait(ev);
		a = ha->atomic();
                if(have) {
                        void ** dp = a->data();
                        EXPECT_TRUE(NULL != dp);
                        EXPECT_TRUE(NULL != *dp);
                        EXPECT_EQ(expect_data,*(static_cast<int *>(*dp)));
                }
        }
        ~AcquirePoolItem()
        {
		EventBasePtr no_event;
                if(a) release(no_event);
        }
        EventBasePtr release(EventBasePtr & by_ev)
        {
                static EventBasePtr empty;
                std::vector<EventBasePtr> vec;
                if(have) a->release(vec,by_ev);
                EventBasePtr res =
                        ( have && (vec.size() > 0)
                        ? vec.front()
                        : empty);
                have = false;
                return res;
        }

        atomic::Atomic * a;
        bool have;
        bool was_held;
};

const int OFluxAtomicPooledTests::pl = 0; // don't care really

TEST_F(OFluxAtomicPooledTests,Simple1) {
        EventBaseSharedPtr ev1_shared (
                (*createfn_next)(EventBase::no_event_shared,NULL,&n_next));
        EventBasePtr ev1 = get_EventBaseSharedPtr(ev1_shared); 
        EventBaseSharedPtr ev2_shared (
                (*createfn_next)(EventBase::no_event_shared,NULL,&n_next));
        EventBasePtr ev2 = get_EventBaseSharedPtr(ev2_shared); 
        EventBaseSharedPtr ev3_shared (
                (*createfn_next)(EventBase::no_event_shared,NULL,&n_next));
        EventBasePtr ev3 = get_EventBaseSharedPtr(ev3_shared); 

        // initialize pool with 2 things

        AcquirePoolItem api1(ev1,1);
        EXPECT_TRUE(api1.have);
        AcquirePoolItem api2(ev2,2);
        EXPECT_TRUE(api2.have);
        AcquirePoolItem api3(ev3,0);
        EXPECT_FALSE(api3.have);
        EventBasePtr ev_r1 = api1.release(ev1);
        EXPECT_EQ(ev_r1,ev3);
        AcquirePoolItem api3a(ev3,1);
        EXPECT_FALSE(api3a.have);
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
