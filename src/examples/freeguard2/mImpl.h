#ifndef MIMPL_H
#define MIMPL_H

#include <cstdlib>

extern void init(int, char * argv[]);
extern void deinit();

struct FLEl {
	int id;
	FLEl * next;
};

// this is a linked list that allows inserts with CAS (sync free)
class FList {
public:
	FList() : head(NULL) {}
	~FList();
	void insert(FLEl * el);
private:
	FLEl * head;
};

#endif
