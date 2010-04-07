#ifndef SPLAYTEST2_M_IMPL_H
#define SPLAYTEST2_M_IMPL_H

#include <boost/shared_ptr.hpp>
#include <iostream>

class Foo {
  public:
    Foo(int foo = 0) : foo_(foo) {
        std::cout << "New Foo: " << this
                  << " foo " << foo_ << std::endl;
    }

    ~Foo() {
        std::cout << "Del Foo: " << this
                  << " foo " << foo_ << std::endl;
    }

    int foo_;
};

typedef boost::shared_ptr<Foo> FooPtr;

#endif
