#pragma once

#include <boost/log/common.hpp>
namespace logging = boost::log;

#include <boost/log/attributes.hpp>
namespace attrs = boost::log::attributes;

#define SEVERITY_ATTR "Severity"
#define TIMESTAMP_ATTR "TimeStamp"
#define SCOPE_ATTR "Scope"
#define PROCESS_NAME_ATTR "ProcessName"
#define PROCESS_ID_ATTR "ProcessID"
#define THREAD_ID_ATTR "ThreadID"
#define ISOLATE_ATTR "Isolate"
#define CONTEXT_ATTR "Context"
#define SCRIPT_NAME_ATTR "ScriptName"
#define SCRIPT_LINE_NO_ATTR "ScriptLineNo"
#define SCRIPT_COLUMN_NO_ATTR "ScriptColumnNo"

enum severity_level
{
    trace,
    debug,
    info,
    warning,
    error,
    fatal
};

#if !defined(BOOST_LOG_NO_THREADS)
typedef boost::log::sources::severity_logger_mt<severity_level> logger_t;
#else
typedef boost::log::sources::severity_logger<severity_level> logger_t;
#endif

extern void initialize_logging();