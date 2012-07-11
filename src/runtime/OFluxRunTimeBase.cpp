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
#include "OFluxRunTimeBase.h"
#include "OFluxLogging.h"
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
		} else if(strcmp(s,"logging") == 0) {
			const char * v = strtok(NULL,delim);
			if(v) {
				size_t l = strlen(v);
				const char * lev_str = NULL;
				for(int i = logging::LL_trace; i < logging::LL_count; ++i) {
					lev_str = logging::convert_level_to_string(static_cast<logging::Level>(i));
					if(strncmp(lev_str,v,l) == 0) {
						logging::logger->setLevelOnOff(static_cast<logging::Level>(i),true);
					}
				}
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
