#ifndef _OFLUX_LOGGING
#define _OFLUX_LOGGING

#include <ostream>

namespace oflux {

/** @file OFluxLogging.h
 *  @author Mark Pichora
 *  This logging module allows you to punt the logging in the runtime
 *    to specified streams
 */

enum LoggingLevel 
	{ LL_info = 0
	, LL_warn = 1
	, LL_error = 2 
	, LL_count = 3 
	};

/**
 * @class LoggingAbstract
 * @brief specifies the abstract interface of a logger (for hooking a framework)
 */
class LoggingAbstract {
public:
	virtual ~LoggingAbstract() {}
	virtual int oflux_log(LoggingLevel,const char *, ...) = 0;
	virtual void setLevelOnOff(LoggingLevel lv, bool to_on) = 0;
};

/**
 * @class LoggingNone
 * @brief implementation of a logger that does no logging
 */
class LoggingNone : public LoggingAbstract {
public:
	static LoggingNone nologger;
	LoggingNone() {}
	virtual ~LoggingNone() {}
	virtual int oflux_log(LoggingLevel,const char *,...) { return 0; }
	virtual void setLevelOnOff(LoggingLevel, bool) {}
};

typedef std::ostream * StreamArray[];

/**
 * @class LoggingStreamed
 * @brief a logger implementation for hooking to output streams (std::ostream)
 */
class LoggingStreamed : public LoggingAbstract {
public:
	LoggingStreamed(StreamArray streams, 
		int levels);
	virtual int oflux_log(LoggingLevel,const char *,...);
	virtual void setLevelOnOff(LoggingLevel lv, bool to_on);
private:
	int _levels;
	std::ostream ** _streams;
	bool is_on[LL_count];
};

/**
 * @brief set all the logging streams to be a single output stream (creates a LoggingStreamed object internally)
 * @param os the given output stream
 */
void loggingToStream(std::ostream & os); // only call this once!

#define oflux_log_info(...) logger->oflux_log(LL_info,__VA_ARGS__)
#define oflux_log_warn(...) logger->oflux_log(LL_warn,__VA_ARGS__)
#define oflux_log_error(...) logger->oflux_log(LL_error,__VA_ARGS__)

} // namespace


#endif // _OFLUX_LOGGING
