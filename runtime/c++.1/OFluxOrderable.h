#ifndef OFLUX_ORDERABLE
#define OFLUX_ORDERABLE

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
        std::vector<Inequality> _inequalities;
};

} // namespace

#endif //OFLUX_ORDERABLE

