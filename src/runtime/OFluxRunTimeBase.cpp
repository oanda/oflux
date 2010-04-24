#include "OFluxRunTimeBase.h"
#include <cstring>
#include <cstdlib>

namespace oflux {

initShimFnType RunTimeBase::initShim = NULL;
deinitShimFnType RunTimeBase::deinitShim = NULL;

EnvironmentVar::EnvironmentVar(int rt_number)
	: nostart(false)
	, runtime_number(rt_number)
{
	// Examples:
	//  export OFLUX_CONFIG=nostart
	//  export OFLUX_CONFIG=runtime_number=1
	//  export OFLUX_CONFIG=runtime_number=4
	static const char * var_name = "OFLUX_CONFIG";
	static const char * delim = ",=";
	char * val = getenv(var_name);
	for(const char * s = strtok(val,delim); s; s = strtok(NULL,delim)) {
		if(strcmp(s,"nostart") == 0) {
			nostart = true;
		} else if(strcmp(s,"runtime_number") == 0) {
			const char * v = strtok(NULL,delim);
			if(v) {
				runtime_number = atoi(v);
			}
		}
	}
}

namespace runtime {

RunTimeAbstract *
Factory::create(int runtime_number, const RunTimeConfiguration & rtc)
{
	RunTimeBase::initShim = (initShimFnType)dlsym (RTLD_NEXT, "initShim");
	RunTimeBase::deinitShim = (deinitShimFnType)dlsym (RTLD_NEXT, "deinitShim");
	if(runtime_number == Factory::lockfree) {
		return _create_lockfree_runtime(rtc);
	} else if(runtime_number == Factory::melding) {
		return _create_melding_runtime(rtc);
	} else { // classic case
		return _create_classic_runtime(rtc);
	}
}

} // namespace runtime

} // namespace oflux
