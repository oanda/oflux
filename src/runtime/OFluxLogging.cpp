#include <stdio.h>
#include <string.h>
#include "OFlux.h"
#include "OFluxLogging.h"
#include <stdarg.h>
#include <stdio.h>
#include <iostream>

namespace oflux {
namespace logging {

None None::nologger;

Abstract * logger = &None::nologger;

static const char * convert_level_to_string(Level lv)
{
	static const char * conv[LL_count] = 
		{ "debug "
                , "info  "
		, "warn  "
		, "error " 
		};
	return conv[lv];
}

Streamed::Streamed(StreamArray streams,
                int levels)
	: _levels(levels)
	, _streams(streams)
{
	assert(levels == LL_count); // levels should coincide
	oflux_log_info("OFlux runtime logging begins (%s)\n"
                , OFLUX_RUNTIME_VERSION);
	for(int i = 0; i < LL_count; i++) {
		is_on[i] = true;
	}
}

#define BUFFERLONGENOUGH 512

int Streamed::oflux_log(Level lv, const char * fmt, ...)
{
	int rc = 0;
	if(is_on[lv]) {
		char buffer[BUFFERLONGENOUGH];
		va_list args;
		va_start (args, fmt);
		int sz = BUFFERLONGENOUGH;
		snprintf(buffer,sz,"%ld %s",
			fast_time(NULL),
			convert_level_to_string(lv));
		int hsz = strlen(buffer);
		sz -= hsz;
		rc = vsnprintf (buffer+hsz,sz,fmt, args);
		std::ostream & strm = *(_streams[lv]);
		va_end (args);
		strm.write(buffer,strlen(buffer));
                strm.flush();
	}
	return rc;
}

void Streamed::setLevelOnOff(Level lv, bool to_on)
{
	is_on[lv] = to_on;
}

void toStream(std::ostream & os) 
{
	static std::ostream * streamarray[LL_count] = 
		{ &os
		, &os
		, &os
		, &os
		};
	static Streamed logging_(streamarray);
	logger = &logging_;
}

} // namespace logging
} // namespace oflux
