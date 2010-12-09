#ifndef OFLUX_ROLLING_LOG
#define OFLUX_ROLLING_LOG

#include <string.h>
#include <strings.h>

// This class is used as portable instrumentation when needed

namespace oflux {

template<typename D> // D is POD
class RollingLog {
public:
	enum { log_size = 4096*4, d_size = sizeof(D) };
	RollingLog()
		: index(1LL)
	{
		::bzero(&(log[0]),sizeof(log));
	}
	inline D & submit()
	{
		long long i = __sync_fetch_and_add(&index,1);
		log[i%log_size].index = i;
		return log[i%log_size].d;
	}
	inline long long at() { return index; }
protected:
	long long index;
	struct S { long long index; D d; } log[log_size];
};

} // namespace oflux

#endif // OFLUX_ROLLING_LOG
