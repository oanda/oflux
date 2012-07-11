/*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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

const char * convert_level_to_string(Level lv)
{
	static const char * conv[LL_count] = 
		{ "trace "
		, "debug "
                , "info  "
		, "warn  "
		, "error " 
		};
	return conv[lv];
}

#define STARTING_LEVEL LL_info
//#define STARTING_LEVEL LL_trace

Streamed::Streamed(StreamArray streams,
                int levels)
	: _levels(levels)
	, _streams(streams)
{
	assert(levels == LL_count); // levels should coincide
	oflux_log_info("OFlux runtime logging begins (%s)\n"
                , OFLUX_RUNTIME_VERSION);
	// trace if off by default
	for(int i = STARTING_LEVEL; i < LL_count; i++) {
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
		snprintf(buffer,sz,
			//"%lld %s", gethrtime(), 
			"%ld %s", fast_time(NULL),
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
		, &os
		};
	static Streamed logging_(streamarray);
	logger = &logging_;
}

} // namespace logging
} // namespace oflux
