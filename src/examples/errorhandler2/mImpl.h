#ifndef MIMPL_H
#define MIMPL_H

extern void init(int, char * argv[]);

class C {
public:
	C(const C & cl) : _c(cl._c) , _cp(&_c) {}
	C() : _c(0), _cp(&_c) {}
	~C() { _c = -1; _cp= (int *)0xdeadbeef; }
	C & operator=(const C & cl) { _c = cl._c; return *this; }
	int get() const { return *_cp; }
	void set(int c) { *_cp = c; }
private:
	int _c;
	int * _cp;
};

#endif
