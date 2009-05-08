#ifndef _OFLUX_LOGGING
#define _OFLUX_LOGGING

#include "OFlux.h"
#include <ostream>

namespace oflux {
namespace logging {

/** @file OFluxLogging.h
 *  @author Mark Pichora
 *  This logging module allows you to punt the logging in the runtime
 *    to specified streams
 */

enum Level 
	{ LL_debug = 0
        , LL_info  = LL_debug+1
	, LL_warn  = LL_info +1
	, LL_error = LL_warn +1
	, LL_count = LL_error+1
	};

/**
 * @class Abstract
 * @brief specifies the abstract interface of a logger (for hooking a framework)
 */
class Abstract {
public:
	virtual ~Abstract() {}
	virtual int oflux_log(Level,const char *, ...) = 0;
	virtual void setLevelOnOff(Level lv, bool to_on) = 0;
};

/**
 * @class None
 * @brief implementation of a logger that does no logging
 */
class None : public Abstract {
public:
	static None nologger;
	None() {}
	virtual ~None() {}
	virtual int oflux_log(Level,const char *,...) { return 0; }
	virtual void setLevelOnOff(Level, bool) {}
};

typedef std::ostream * StreamArray[];

/**
 * @class Streamed
 * @brief a logger implementation for hooking to output streams (std::ostream)
 */
class Streamed : public Abstract {
public:
	Streamed(StreamArray streams, 
		int levels = LL_count);
	virtual int oflux_log(Level,const char *,...);
	virtual void setLevelOnOff(Level lv, bool to_on);
private:
	int _levels;
	std::ostream ** _streams;
	bool is_on[LL_count];
};

/**
 * @brief set all the logging streams to be a single output stream (creates a LoggingStreamed object internally)
 * @param os the given output stream
 */
void toStream(std::ostream & os); // only call this once!

#define oflux_log_debug(...) \
        oflux::logging::logger->oflux_log(oflux::logging::LL_debug,__VA_ARGS__)
#define oflux_log_info(...) \
        oflux::logging::logger->oflux_log(oflux::logging::LL_info,__VA_ARGS__)
#define oflux_log_warn(...) \
        oflux::logging::logger->oflux_log(oflux::logging::LL_warn,__VA_ARGS__)
#define oflux_log_error(...) \
        oflux::logging::logger->oflux_log(oflux::logging::LL_error,__VA_ARGS__)

} // namespace logging
} // namespace oflux


#endif // _OFLUX_LOGGING
