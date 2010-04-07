#include "CommonEventunit.h"

using namespace oflux;

class OFluxAtomicTests : public OFluxCommonEventTests {
public:
	OFluxAtomicTests(atomic::Atomic *);
        virtual ~OFluxAtomicTests() {}
	virtual void SetUp() {}
	virtual void TearDown() {}

        void checkRelease(boost::shared_ptr<EventBase> & ev);

        CreateNodeFn createfn_source;
        CreateNodeFn createfn_succ;
        CreateNodeFn createfn_next;

        atomic::Atomic * _atomic_ptr;
};

void OFluxAtomicTests::checkRelease(boost::shared_ptr<EventBase> & ev)
{
        std::vector<boost::shared_ptr<EventBase> > rel_ev;
        _atomic_ptr->release(rel_ev);
        EXPECT_TRUE((ev.get() == NULL && rel_ev.size() == 0)
                || 
                (rel_ev.size() == 1 && rel_ev.front() == ev));
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
        boost::shared_ptr<EventBase> ev_source =
                (*createfn_source)(EventBase::no_event,NULL,&n_source);
        boost::shared_ptr<EventBase> ev_succ =
                (*createfn_succ)(ev_source,NULL,&n_succ);
        boost::shared_ptr<EventBase> ev_next =
                (*createfn_next)(ev_succ,NULL,&n_next);

        EXPECT_FALSE(atom.held());
        EXPECT_TRUE (atom.acquire(excl));
        EXPECT_TRUE (atom.held());
        EXPECT_FALSE(atom.acquire(excl));
        EXPECT_EQ(atom.waiter_count(),0);
        atom.wait(ev_source,excl);
        atom.wait(ev_succ,excl);
        atom.wait(ev_next,excl);
        EXPECT_EQ(atom.waiter_count(),3);
        checkRelease(ev_source);
        EXPECT_TRUE (atom.acquire(excl));
        EXPECT_FALSE(atom.acquire(excl));
        EXPECT_EQ(atom.waiter_count(),2);
        checkRelease(ev_succ);
        EXPECT_TRUE (atom.acquire(excl));
        EXPECT_FALSE(atom.acquire(excl));
        EXPECT_EQ(atom.waiter_count(),1);
        checkRelease(ev_next);
        EXPECT_TRUE (atom.acquire(excl));
        EXPECT_FALSE(atom.acquire(excl));
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
        EventBasePtr ev_nothing;
        EventBasePtr ev_any_reader =
                (*createfn_next)(EventBase::no_event,NULL,&n_next);
        EventBasePtr ev_writer =
                (*createfn_source)(EventBase::no_event,NULL,&n_source);

        // 2 readers added:
        EXPECT_EQ(0,atom.held());
        EXPECT_TRUE(atom.acquire(read));
        EXPECT_EQ(1,atom.held());
        EXPECT_TRUE(atom.acquire(read));
        EXPECT_EQ(2,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        // 1 writer queued:
        EXPECT_FALSE(atom.acquire(write));
        atom.wait(ev_writer,write);
        EXPECT_EQ(2,atom.held());
        EXPECT_EQ(atom.waiter_count(),1);
        // release 2 readers
        checkRelease(ev_nothing);
        EXPECT_EQ(1,atom.held());
        checkRelease(ev_writer);
        EXPECT_EQ(atom.waiter_count(),0);
        EXPECT_EQ(0,atom.held());
        // writer acquires
        EXPECT_TRUE (atom.acquire(write));
        EXPECT_EQ(1,atom.held());
        EXPECT_FALSE(atom.acquire(read));
        checkRelease(ev_nothing);
        EXPECT_EQ(0,atom.held());
}

TEST_F(OFluxAtomicReadWriteTests,ReadWriteRead1) {
        EventBasePtr ev_nothing;
        EventBasePtr ev_reader =
                (*createfn_next)(EventBase::no_event,NULL,&n_next);
        EventBasePtr ev_writer =
                (*createfn_source)(EventBase::no_event,NULL,&n_source);

        // 1 readers added:
        EXPECT_EQ(0,atom.held());
        EXPECT_TRUE(atom.acquire(read));
        EXPECT_EQ(1,atom.held());
        // 1 writer must wait
        EXPECT_FALSE(atom.acquire(write));
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        atom.wait(ev_writer,write);
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),1);
        // 1 reader must wait
        EXPECT_FALSE(atom.acquire(read))
                << "should fail since one writer waits";
        EXPECT_EQ(1,atom.held());
        atom.wait(ev_reader,read);
        EXPECT_EQ(atom.waiter_count(),2);
        EXPECT_EQ(1,atom.held());
        // writer succeeds
        checkRelease(ev_writer);
        EXPECT_EQ(atom.waiter_count(),1);
        EXPECT_EQ(0,atom.held());
        EXPECT_TRUE(atom.acquire(write));
        EXPECT_EQ(1,atom.held());
        // writer releases
        checkRelease(ev_reader);
        EXPECT_EQ(0,atom.held());
        // second reader acquires
        EXPECT_TRUE(atom.acquire(read));
        EXPECT_EQ(atom.waiter_count(),0);
        EXPECT_EQ(1,atom.held());
        checkRelease(ev_nothing);
        EXPECT_EQ(0,atom.held());
}

TEST_F(OFluxAtomicReadWriteTests,WriteReadRead1) {
        EventBasePtr ev_nothing;
        EventBasePtr ev_reader1 =
                (*createfn_next)(EventBase::no_event,NULL,&n_next);
        EventBasePtr ev_reader2 =
                (*createfn_source)(EventBase::no_event,NULL,&n_source);

        // 1 writer added:
        EXPECT_EQ(0,atom.held());
        EXPECT_TRUE(atom.acquire(write));
        EXPECT_EQ(1,atom.held());
        // 1 reader must wait
        EXPECT_FALSE(atom.acquire(read));
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),0);
        atom.wait(ev_reader1,read);
        EXPECT_EQ(1,atom.held());
        EXPECT_EQ(atom.waiter_count(),1);
        // 2nd reader must wait
        EXPECT_FALSE(atom.acquire(read));
        EXPECT_EQ(1,atom.held());
        atom.wait(ev_reader2,read);
        EXPECT_EQ(atom.waiter_count(),2);
        EXPECT_EQ(1,atom.held());
        // writer releases
        std::vector<boost::shared_ptr<EventBase> > rel_ev;
        atom.release(rel_ev);
        ASSERT_EQ(2,rel_ev.size());
        EXPECT_EQ(rel_ev[0],ev_reader1);
        EXPECT_EQ(rel_ev[1],ev_reader2);
        EXPECT_EQ(atom.waiter_count(),0);
        EXPECT_EQ(0,atom.held());
        EXPECT_TRUE(atom.acquire(read));
        EXPECT_TRUE(atom.acquire(read));
        EXPECT_EQ(2,atom.held());
        // both readers release
        checkRelease(ev_nothing);
        checkRelease(ev_nothing);
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
                // mimic populator
                pool.get(atom1,NULL);
                void ** d1 = atom1->data();
                *d1 = &data1_;
                pool.get(atom2,NULL);
                void ** d2 = atom2->data();
                *d2 = &data2_;
                std::vector<boost::shared_ptr<EventBase> > vec;
                atom1->release(vec);
                atom2->release(vec);
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
        AcquirePoolItem(atomic::AtomicPool & pool
                , EventBasePtr & ev
                , int expect_data)
        {
                pool.get(a,NULL);
                EXPECT_TRUE(NULL != a);
                was_held = a->held();
                have = a->acquire(OFluxAtomicPooledTests::pl);
                if(!have) {
                        a->wait(ev,OFluxAtomicPooledTests::pl);
                } else {
                        void ** dp = a->data();
                        EXPECT_TRUE(NULL != dp);
                        EXPECT_TRUE(NULL != *dp);
                        EXPECT_EQ(expect_data,*(static_cast<int *>(*dp)));
                }
        }
        ~AcquirePoolItem()
        {
                if(a) release();
        }
        boost::shared_ptr<EventBase> release()
        {
                static boost::shared_ptr<EventBase> empty;
                std::vector<boost::shared_ptr<EventBase> > vec;
                if(have) a->release(vec);
                boost::shared_ptr<EventBase> res =
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
        EventBasePtr ev_nothing;
        EventBasePtr ev1 =
                (*createfn_next)(EventBase::no_event,NULL,&n_next);
        EventBasePtr ev2 =
                (*createfn_next)(EventBase::no_event,NULL,&n_next);
        EventBasePtr ev3 =
                (*createfn_next)(EventBase::no_event,NULL,&n_next);

        // initialize pool with 2 things

        AcquirePoolItem api1(pool,ev1,1);
        EXPECT_TRUE(api1.have);
        AcquirePoolItem api2(pool,ev2,2);
        EXPECT_TRUE(api2.have);
        AcquirePoolItem api3(pool,ev3,0);
        EXPECT_FALSE(api3.have);
        boost::shared_ptr<EventBase> ev_r1 = api1.release();
        EXPECT_EQ(ev_r1,ev3);
        AcquirePoolItem api3a(pool,ev3,1);
        EXPECT_TRUE(api3a.have);
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
