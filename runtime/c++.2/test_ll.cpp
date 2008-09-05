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
#include <iostream>

using namespace oflux;

class C {
public:
	C(int c) : _c(c) {}
	~C() { _c = -1; }
	int cf() { return _c; }
	static void pp(C * c) { std::cout << c->cf() << ", "; }
private:
	int _c;
};

class B : public Removable<B, InheritedNode<B> > {
public:
	B(int c) : _c(c) {}
	~B() { _c = -1; }
	int cf() { return _c; }
	static void pp(B * c) { std::cout << c->cf() << ", "; }
private:
	int _c;
};

template<class C, class N >
void pp_list(LinkedListRemovable<C,N> * llp)
{
	llp->iter(C::pp);
}

typedef Removable<C, Node<C> > RNC;

int main1()
{
	LinkedListRemovable<C,RNC> ll;

	C c1(1);
	C c2(2);
	C c3(3);
	C c4(4);
	C c5(5);

	RNC * n1 = ll.insert_front(&c1);
	pp_list<C, RNC >(&ll); std::cout << std::endl;
	RNC * n2 = ll.insert_front(&c2);
	pp_list<C, RNC>(&ll); std::cout << std::endl;
	RNC * n3 = ll.insert_back(&c3);
	pp_list<C, RNC>(&ll); std::cout << std::endl;
	RNC * n4 = ll.insert_front(&c4);
	pp_list<C, RNC>(&ll); std::cout << std::endl;
	RNC * n5 = ll.insert_back(&c5);
	pp_list<C, RNC>(&ll); std::cout << std::endl;

	ll.remove(n3);
	pp_list<C, RNC >(&ll); std::cout << std::endl;
	ll.remove(n1);
	pp_list<C, RNC >(&ll); std::cout << std::endl;
	ll.remove(n2);
	ll.remove(n4);
	ll.remove(n5);
	pp_list<C, RNC >(&ll); std::cout << std::endl;

	return 0;
}

typedef Removable<C, SharedPtrNode<C> > RSNPC;

int main3()
{
	LinkedListRemovable<C,RSNPC> ll;

	RSNPC * n1 = NULL;
	RSNPC * n2 = NULL;
	RSNPC * n3 = NULL;
	RSNPC * n4 = NULL;
	RSNPC * n5 = NULL;

	{ // manage lifetimes
		boost::shared_ptr<C> c1(new C(1));
		boost::shared_ptr<C> c2(new C(2));
		boost::shared_ptr<C> c3(new C(3));
		boost::shared_ptr<C> c4(new C(4));
		boost::shared_ptr<C> c5(new C(5));

		n1 = ll.insert_front(c1);
		pp_list<C,RSNPC>(&ll); std::cout << std::endl;
		n2 = ll.insert_front(c2);
		pp_list<C,RSNPC>(&ll); std::cout << std::endl;
		n3 = ll.insert_back(c3);
		pp_list<C,RSNPC>(&ll); std::cout << std::endl;
		n4 = ll.insert_front(c4);
		pp_list<C,RSNPC>(&ll); std::cout << std::endl;
		n5 = ll.insert_back(c5);
		pp_list<C,RSNPC>(&ll); std::cout << std::endl;
	}

	boost::shared_ptr<C> cs1(n1->shared_content());
	ll.remove(n3);
	pp_list<C,RSNPC>(&ll); std::cout << std::endl;
	ll.remove(n1);
	pp_list<C,RSNPC>(&ll); std::cout << std::endl;
	ll.remove(n2);
	ll.remove(n4);
	ll.remove(n5);
	pp_list<C,RSNPC>(&ll); std::cout << std::endl;
	return 0;
}

typedef Removable<B, InheritedNode<B> > RNB;

int main2()
{
	LinkedListRemovable<B,RNB> ll;

	B c1(1);
	B c2(2);
	B c3(3);
	B c4(4);
	B c5(5);

	RNB * n1 = ll.insert_front(&c1);
	pp_list<B,RNB>(&ll); std::cout << std::endl;
	RNB * n2 = ll.insert_front(&c2);
	pp_list<B,RNB>(&ll); std::cout << std::endl;
	RNB * n3 = ll.insert_back(&c3);
	pp_list<B,RNB>(&ll); std::cout << std::endl;
	RNB * n4 = ll.insert_front(&c4);
	pp_list<B,RNB>(&ll); std::cout << std::endl;
	RNB * n5 = ll.insert_back(&c5);
	pp_list<B,RNB>(&ll); std::cout << std::endl;

	ll.remove(n3);
	pp_list<B,RNB>(&ll); std::cout << std::endl;
	ll.remove(n1);
	pp_list<B,RNB>(&ll); std::cout << std::endl;
	ll.remove(n2);
	ll.remove(n4);
	ll.remove(n5);
	pp_list<B,RNB>(&ll); std::cout << std::endl;
	return 0;
}
				
int main4()
{
	LinkedListRemovable<B,RNB> ll1;
	LinkedListRemovable<B,RNB> ll2;

	B c1(1);
	B c2(2);
	B c3(3);
	B c4(4);
	B c5(5);

	RNB * n1 = ll1.insert_front(&c1);
	RNB * n2 = ll1.insert_front(&c2);
	RNB * n3 = ll1.insert_front(&c3);

	RNB * n4 = ll2.insert_front(&c4);
	RNB * n5 = ll2.insert_front(&c5);

	std::cout << "insert_front_all... before & after" << std::endl;
	pp_list<B,RNB>(&ll1); std::cout << std::endl;
	pp_list<B,RNB>(&ll2); std::cout << std::endl;
	ll1.insert_front_all(ll2);
	pp_list<B,RNB>(&ll1); std::cout << std::endl;
	pp_list<B,RNB>(&ll2); std::cout << std::endl;
}

int main5()
{
	LinkedListRemovable<B,RNB> ll1;
	LinkedListRemovable<B,RNB> ll2;

	B c1(1);
	B c2(2);
	B c3(3);
	B c4(4);
	B c5(5);

	RNB * n1 = ll1.insert_front(&c1);
	RNB * n2 = ll1.insert_front(&c2);
	RNB * n3 = ll1.insert_front(&c3);

	RNB * n4 = ll2.insert_front(&c4);
	RNB * n5 = ll2.insert_front(&c5);

	std::cout << "insert_back_all... before & after" << std::endl;
	pp_list<B,RNB>(&ll1); std::cout << std::endl;
	pp_list<B,RNB>(&ll2); std::cout << std::endl;
	ll1.insert_back_all(ll2);
	pp_list<B,RNB>(&ll1); std::cout << std::endl;
	pp_list<B,RNB>(&ll2); std::cout << std::endl;
}

int main()
{
	main1();
	main2();
	main3();
	main4();
	main5();
	return 0;
}
