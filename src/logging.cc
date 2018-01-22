/**
 * \file
 * Implementation for log messages.
 */

#include "elasticlient/logging.h"

#include "logging-impl.h"


namespace elasticlient {


/// LogCallback function
static LogCallback logFunction = nullptr;


bool isLoggingEnabled() {
    return logFunction != nullptr;
}


void setLogFunction(LogCallback extLogFunction) {
    logFunction = extLogFunction;
}


void dbgLog(LogLevel logLevel, const std::string &message) {
    if (logFunction) {
        logFunction(logLevel, message);
    }
}


} // namespace elasticlient
