#ifndef _MIMPL_CHAINGUARD2_H
#define _MIMPL_CHAINGUARD2_H

class Aob {
public:
	Aob(int a) : _a(a) {}

	int
	get_a() const { return _a; }
private:
	int _a;
};

class Bob {
public:
	Bob(int b) : _b(b) {}

	int
	get_b() const { return _b; }

	void
	add(int val) { _b += val; }
private:
	int _b;
};

// fwd decl of Cob since we're not accessing its members in the flow
class Cob;

#endif // _MIMPL_CHAINGUARD2_H
