#ifdef HAS_DTRACE
# include "ofluxprobe.h"
#endif

extern "C" {

void trigger_probe_aha_exception_begin_throw(int i)
{
#ifdef HAS_DTRACE
	OFLUX_AHA_EXCEPTION_BEGIN_THROW(i);
#else
#endif
}

}; // extern "C"

