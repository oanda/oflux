#ifndef IMPL_SCATTERGATHER_H
#define IMPL_SCATTERGATHER_H

#include <iostream>
#include "OFluxScatterGather.h"


class GatherObj : public oflux::Gatherable {
public:
	GatherObj(size_t sz)
		: _arr(new int[sz])
		, _sz(sz)
	{
		std::cout << " GatherObj created" << std::endl;
	}
	~GatherObj()
	{
		std::cout << " ~GatherObj called" << std::endl;
		delete [] _arr;
		_arr = NULL;
	}
	void put(size_t ind, int res)
	{
		_arr[ind] = res;
	}
	void dump() const
	{
		for(size_t i = 0; i < _sz; ++i) {
			std::cout << "arr [" << i << "] = "
				<< _arr[i] << std::endl;
		}
	}
private:
	int * _arr;
	size_t _sz;
};

#endif
