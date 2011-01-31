#include "OFluxLibDTrace.h"

extern "C" {

void trigger_probe_aha_exception_begin_throw(int i)
{
	_AHA_EXCEPTION_BEGIN_THROW(i);
}

}; // extern "C"

