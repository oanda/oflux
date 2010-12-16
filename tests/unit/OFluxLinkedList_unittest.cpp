/* 
  of interest, and for your consideration:
  valgrind --leak-check=full bin/test_ll 

output:
1,
2, 1,
2, 1, 3,
4, 2, 1, 3,
4, 2, 1, 3, 5,
4, 2, 1, 5,
4, 2, 5,

1,
2, 1,
2, 1, 3,
4, 2, 1, 3,
4, 2, 1, 3, 5,
4, 2, 1, 5,
4, 2, 5,

1,
2, 1,
2, 1, 3,
4, 2, 1, 3,
4, 2, 1, 3, 5,
4, 2, 1, 5,
4, 2, 5,

*/
#include "OFluxLinkedList.h"
#include <gtest/gtest.h>

namespace oflux {
 class RunTimeAbstractForShim; 
} // namespace oflux

oflux::RunTimeAbstractForShim *eminfo = NULL; // need this symbol due to doors

using namespace oflux;

class C {
public:
	C(int c) : _c(c) {}
	~C() { _c = -1; }
	int cf() { return _c; }
	//static void pp(C * c) { std::cout << c->cf() << ", "; }
private:
	int _c;
};

class B : public Removable<B, InheritedNode<B> > {
public:
	B(int c) : _c(c) {}
	~B() { _c = -1; }
	int cf() { return _c; }
	//static void pp(B * c) { std::cout << c->cf() << ", "; }
private:
	int _c;
};

typedef int ContentType;

struct FPT {
        int sz;
        const int max_sz;
        const ContentType * ctptr;
};

template<class X>
void * contents_fold_fun(void *fp_vd, X * c)
{
        FPT * fp = static_cast<FPT *>(fp_vd);
        EXPECT_LE(fp->sz,fp->max_sz)
                << "exceeded the maximum size in content check";
        EXPECT_EQ(*(fp->ctptr),c->cf())
                << "content check on single element";
        (fp->sz)++;
        (fp->ctptr)++;
        return fp_vd;
}

template<const int csz, class LinkedListType>
void validate_contents_templ(
          ContentType arr[]
        , LinkedListType & ll)
{
        FPT fpt = { 0, csz, &arr[0] };
        ll.fold(&fpt,&(contents_fold_fun<typename LinkedListType::content_type>));
        EXPECT_EQ(fpt.sz,csz)
                << "content size mismatch";
}

#define validate_contents(LL,...) \
{ \
  ContentType arr[] = { __VA_ARGS__ }; \
  validate_contents_templ<sizeof(arr)/sizeof(ContentType),typeof(LL)>(arr,LL); \
}

class OFluxLinkedListTests : public testing::Test {
public:
        OFluxLinkedListTests() {}
        virtual ~OFluxLinkedListTests() {}
        virtual void SetUp() {}
        virtual void TearDown() {}
public:
};


/*
template<class C, class N >
void pp_list(LinkedListRemovable<C,N> * llp)
{
llp->iter(C::pp);
}
*/

typedef Removable<C, Node<C> > RNC;

TEST_F(OFluxLinkedListTests,Test1) {
        LinkedListRemovable<C,RNC> ll;

        C c1(1);
        C c2(2);
        C c3(3);
        C c4(4);
        C c5(5);

        RNC * n1 = ll.insert_front(&c1);
        validate_contents(ll, 1);
        RNC * n2 = ll.insert_front(&c2);
        validate_contents(ll, 2, 1);
        RNC * n3 = ll.insert_back(&c3);
        validate_contents(ll, 2, 1, 3);
	RNC * n4 = ll.insert_front(&c4);
        validate_contents(ll, 4, 2, 1, 3);
	RNC * n5 = ll.insert_back(&c5);
        validate_contents(ll, 4, 2, 1, 3, 5);

	ll.remove(n3);
        validate_contents(ll, 4, 2, 1, 5);
	ll.remove(n1);
        validate_contents(ll, 4, 2, 5);
	ll.remove(n2);
        validate_contents(ll, 4, 5);
	ll.remove(n4);
        validate_contents(ll, 5);
	ll.remove(n5);
        validate_contents(ll);
}

typedef Removable<C, SharedPtrNode<C> > RSNPC;

TEST_F(OFluxLinkedListTests,Test2) {
	LinkedListRemovable<C,RSNPC> ll;

	RSNPC * n1 = NULL;
	RSNPC * n2 = NULL;
	RSNPC * n3 = NULL;
	RSNPC * n4 = NULL;
	RSNPC * n5 = NULL;

	{ // manage lifetimes
		shared_ptr<C> c1(new C(1));
		shared_ptr<C> c2(new C(2));
		shared_ptr<C> c3(new C(3));
		shared_ptr<C> c4(new C(4));
		shared_ptr<C> c5(new C(5));

                validate_contents(ll);
		n1 = ll.insert_front(c1);
                validate_contents(ll, 1);
		n2 = ll.insert_front(c2);
                validate_contents(ll, 2, 1);
		n3 = ll.insert_back(c3);
                validate_contents(ll, 2, 1, 3);
		n4 = ll.insert_front(c4);
                validate_contents(ll, 4, 2, 1, 3);
		n5 = ll.insert_back(c5);
                validate_contents(ll, 4, 2, 1, 3, 5);
	}

	shared_ptr<C> cs1(n1->shared_content());
        validate_contents(ll, 4, 2, 1, 3, 5);
	ll.remove(n3);
        validate_contents(ll, 4, 2, 1, 5);
	ll.remove(n1);
        validate_contents(ll, 4, 2, 5);
	ll.remove(n2);
        validate_contents(ll, 4, 5);
	ll.remove(n4);
        validate_contents(ll, 5);
	ll.remove(n5);
        validate_contents(ll);
}

typedef Removable<B, InheritedNode<B> > RNB;

TEST_F(OFluxLinkedListTests,Test3) {
	LinkedListRemovable<B,RNB> ll;

	B c1(1);
	B c2(2);
	B c3(3);
	B c4(4);
	B c5(5);

        validate_contents(ll);
	RNB * n1 = ll.insert_front(&c1);
        validate_contents(ll,1);
	RNB * n2 = ll.insert_front(&c2);
        validate_contents(ll,2,1);
	RNB * n3 = ll.insert_back(&c3);
        validate_contents(ll,2,1,3);
	RNB * n4 = ll.insert_front(&c4);
        validate_contents(ll,4,2,1,3);
	RNB * n5 = ll.insert_back(&c5);
        validate_contents(ll,4,2,1,3,5);

	ll.remove(n3);
        validate_contents(ll,4,2,1,5);
	ll.remove(n1);
        validate_contents(ll,4,2,5);
	ll.remove(n2);
        validate_contents(ll,4,5);
	ll.remove(n4);
        validate_contents(ll,5);
	ll.remove(n5);
        validate_contents(ll);
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
