#include "OFluxOrderable.h"
#include <gtest/gtest.h>
#include <string>
#include <map>

using namespace oflux;

class MS : public MagicSorter {
public:
        typedef std::map<const char *, MagicNumberable* > IMap;
        virtual MagicNumberable * getMagicNumberable(const char * c)
                { 
                IMap::iterator itr = _map.find(c);
                return (itr == _map.end() ? NULL : (*itr).second); 
                }

        void validate_assignment() 
                {
                        IMap::iterator itr = _map.begin();
                        while(itr != _map.end()) {
                                ASSERT_GT((*itr).second->magic_number(),0)
                                        << "should have an assignment for "
                                        << (*itr).first << std::endl;
                                itr++;
                        }
                }
        inline void validate_lt(const char * c1, const char * c2)
                {
                        IMap::iterator itr1 = _map.find(c1);
                        IMap::iterator itr2 = _map.find(c2);
                        ASSERT_TRUE(itr1 != _map.end())
                                << "should have element at " 
                                << c1 << std::endl;
                        ASSERT_TRUE(itr2 != _map.end())
                                << "should have element at " 
                                << c2 << std::endl;
                        EXPECT_LT(itr1->second->magic_number()
                                , itr2->second->magic_number())
                                << "expected " << itr1->first
                                << " and " << itr2->first
                                << " to have .<. assignments\n";
                }
        inline void add(const char *c) 
                {
                        addPoint(c);
                        _map[c] = new MagicNumberable();
                }
        inline void add_gt(const char *c1,const char * c2)
                {
                        IMap::iterator itr1 = _map.find(c1);
                        IMap::iterator itr2 = _map.find(c2);
                        EXPECT_TRUE(itr1 != _map.end())
                                << "should have " << c1 << "in this ms\n";
                        EXPECT_TRUE(itr2 != _map.end())
                                << "should have " << c2 << "in this ms\n";
                        addInequality(c1,c2);
                }

        IMap _map;
};

class OFluxOrderableTests : public testing::Test {
public:
	OFluxOrderableTests() {}
        virtual ~OFluxOrderableTests() {}
	virtual void SetUp() { ms.add(a); ms.add(b); }
	virtual void TearDown() {}
public:
        MS ms0; // empty one
        MS ms;
        static const char * a;
        static const char * b;
        static const char * c;
        static const char * d;
};

const char * OFluxOrderableTests::a = "aaaa";
const char * OFluxOrderableTests::b = "bbbb";
const char * OFluxOrderableTests::c = "cccc";
const char * OFluxOrderableTests::d = "dddd";

TEST_F(OFluxOrderableTests,NumberTwoElements_no_constraints) {
        ms.numberAll();
        ms.validate_assignment();
}

TEST_F(OFluxOrderableTests,NumberFourElements_no_constraints) {
        ms.add(c);
        ms.add(d);
        ms.numberAll();
        ms.validate_assignment();
}

TEST_F(OFluxOrderableTests,NumberTwoElements_single_constraint1) {
        ms.addInequality(a,b);
        ms.numberAll();
        ms.validate_assignment();
        ms.validate_lt(a,b);

}

TEST_F(OFluxOrderableTests,NumberFourElements_single_constraint1) {
        ms.addInequality(a,b);
        ms.add(c);
        ms.add(d);
        ms.numberAll();
        ms.validate_assignment();
        ms.validate_lt(a,b);
}

TEST_F(OFluxOrderableTests,NumberTwoElements_single_constraint2) {
        ms.addInequality(b,a);
        ms.numberAll();
        ms.validate_assignment();
        ms.validate_lt(b,a);
}

TEST_F(OFluxOrderableTests,NumberFourElements_two_constraint1) {
        ms.addInequality(a,b);
        ms.add(c);
        ms.add(d);
        ms.addInequality(d,c);
        ms.numberAll();
        ms.validate_assignment();
        ms.validate_lt(a,b);
        ms.validate_lt(d,c);
}

TEST_F(OFluxOrderableTests,NumberFourElements_three_constraint1) {
        ms.addInequality(a,b);
        ms.add(c);
        ms.addInequality(c,b);
        ms.add(d);
        ms.addInequality(d,c);
        ms.numberAll();
        ms.validate_assignment();
        ms.validate_lt(a,b);
        ms.validate_lt(d,c);
        ms.validate_lt(c,b);
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
