#ifndef OFLUX_ORDERABLE
#define OFLUX_ORDERABLE

/**
 * @file OFluxOrderable.h
 * @author Mark Pichora
 *  These classes define the properties things that are
 *  numberable (in the sense that they can be assigned integral values
 *  at runtime) based on a given order.
 *  The sorter is the thing that figures out whether the problem is
 *  solvable and if so applies its solution (assigning magic numbers)
 */

#include <vector>

namespace oflux {

class MagicNumberable {
public:
        friend class MagicSorter;
        MagicNumberable()
                : _magic_number(0)
                {}
        inline int magic_number() { return _magic_number; }
protected:
        inline void magic_number(int m) { _magic_number = m; }
protected:
        int _magic_number;
};

class MagicSorter {
public:
        MagicSorter() {}
        void numberAll();
        virtual MagicNumberable * getMagicNumberable(const char * c) = 0;
        void addInequality(const char * before, const char * after);
        void addPoint(const char * pt) { _points.push_back(pt); }
protected:
        struct Inequality {
                Inequality(MagicNumberable *b, MagicNumberable *a)
                        : before(b)
                        , after(a)
                        {}

                MagicNumberable * before;
                MagicNumberable * after;
        };
private:
        std::vector<Inequality>  _inequalities;
        std::vector<const char*> _points;
};

} // namespace

#endif //OFLUX_ORDERABLE

