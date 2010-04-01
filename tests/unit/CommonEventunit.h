#ifndef COMMONEVENTUNIT_H
#define COMMONEVENTUNIT_H

#include "OFluxEvent.h"
#include "OFluxFlow.h"
#include <gtest/gtest.h>


class AtomsEmpty {
public:
        void fill(oflux::AtomicsHolder * ah) {}
};

class Empty : public oflux::BaseOutputStruct<Empty> { 
public:
        bool operator==(const Empty &) const { return true; }
	typedef Empty base_type;
};

std::ostream & operator<<(std::ostream & os, const Empty &);

class Data1 : public oflux::BaseOutputStruct<Data1> {
public:
  typedef Data1 base_type;
  int x;
  long y;
  bool operator==(const Data1 &o) const{ return (o.x==x) && (o.y==y); }
};

extern Data1 g_out1;

std::ostream & operator<<(std::ostream & os, const Data1 &d);

class Data2 : public oflux::BaseOutputStruct<Data2> {
public:
  typedef Data2 base_type;
  int y;
  bool operator==(const Data1 &o) const { return (o.y==y); }
};

extern Data2 g_out2;

std::ostream & operator<<(std::ostream & os, const Data2 &d);

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

int f_source(const Empty *, Data1 *out, AtomsEmpty *);
int f_succ(const Data1 *, Data2 *out, AtomsEmpty *);
int f_next(const Data2 *, Empty *, AtomsEmpty *);

class OFluxCommonEventTests : public testing::Test {
public:
	OFluxCommonEventTests();
        virtual ~OFluxCommonEventTests() {}
	
	static oflux::CreateNodeFn c_source;
	static oflux::CreateNodeFn c_succ;
	static oflux::CreateNodeFn c_next;

//private:
	oflux::flow::Node n_source;
	oflux::flow::Node n_succ;
	oflux::flow::Node n_next;
};

#endif // COMMONEVENTUNIT_H
