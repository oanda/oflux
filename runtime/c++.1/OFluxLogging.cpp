#include "OFlux.h"
#include "OFluxLogging.h"
#include <stdarg.h>
#include <iostream>

namespace oflux {

LoggingNone LoggingNone::nologger;

LoggingAbstract * logger = &LoggingNone::nologger;

static const char * convert_level_to_string(LoggingLevel lv)
{
	static const char * conv[LL_count] = 
		{ "info  "
		, "warn  "
		, "error " 
		};
	return conv[lv];
}

LoggingStreamed::LoggingStreamed(StreamArray streams,
                int levels)
	: _levels(levels)
	, _streams(streams)
{
	assert(levels == LL_count); // levels should coincide
	oflux_log(LL_info,"OFlux runtime logging begins (%s)\n",OFLUX_RUNTIME_VERSION);
	for(int i = 0; i < LL_count; i++) {
		is_on[i] = true;
	}
}

#define BUFFERLONGENOUGH 512

int LoggingStreamed::oflux_log(LoggingLevel lv, const char * fmt, ...)
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
		strm << buffer;
	}
	return rc;
}

void LoggingStreamed::setLevelOnOff(LoggingLevel lv, bool to_on)
{
	is_on[lv] = to_on;
}

void loggingToStream(std::ostream & os) 
{
	static std::ostream * streamarray[LL_count] = 
		{ &os
		, &os
		, &os
		};
	static LoggingStreamed logging(streamarray,LL_count);
	logger = &logging;
}

} // namespace
