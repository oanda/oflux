#ifndef _MIMPL_CHAINGUARD1_H
#define _MIMPL_CHAINGUARD1_H

class Aob {
public:
	Aob(int id) : _id(id) {}

	int 
	id() const { return _id; }
private:
	int _id;
};

class Bob;

#endif // _MIMPL_CHAINGUARD_H
