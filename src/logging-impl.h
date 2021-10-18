/**
 * \file
 * Internal implementation of for log messages.
 */

#pragma once

#include "elasticlient/logging.h"

#include <memory>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>

#ifdef __GNUC__ 
#define LOG(logLevel, args ...)                                    \
    do {                                                          \
        if(elasticlient::isLoggingEnabled()) {                    \
            elasticlient::propagateLogMessage(logLevel, args);    \
        }                                                         \
    } while (false)
#else
#define LOG(logLevel, ...)                                    \
    do {                                                          \
        if(elasticlient::isLoggingEnabled()) {                    \
            elasticlient::propagateLogMessage(logLevel, __VA_ARGS__);    \
        }                                                         \
    } while (false)
#endif



namespace elasticlient {


/// Return true if logging callback was set.
bool isLoggingEnabled();


/// Will call specified log callback function with \p logLevel and \p message
void dbgLog(LogLevel logLevel, const std::string &message);


inline void propagateLogMessage(
        LogLevel logLevel, const char* message, ...) 
#ifdef __GNUC__ 
	__attribute__ ((format (printf, 2, 3)))
#endif
;

void propagateLogMessage(LogLevel logLevel, const char* message, ...) {
    va_list args;
    va_start(args, message);
    va_list argsCopy;
    va_copy(argsCopy, args);
    // Try create formatted string in buffer of constant length
    static const int staticBufferSize = 1024;
    char staticBuffer[staticBufferSize];
    int length = std::vsnprintf(staticBuffer, staticBufferSize, message, args);
    va_end(args);
    if (length <= 0) {
        va_end(argsCopy);
        return;
    } else if (length < staticBufferSize) {
        va_end(argsCopy);
        dbgLog(logLevel, std::string(staticBuffer, length));
        return;
    }
    // If it is not possible to create formatted string in constant length buffer
    // make new buffer with sufficient length
    try {
        std::unique_ptr<char[]> dynamicBuffer(new char[length+1]);
        std::vsnprintf(dynamicBuffer.get(), length + 1, message, argsCopy);
        dbgLog(logLevel, std::string(dynamicBuffer.get()));
    } catch(const std::exception &ex) {
        va_end(argsCopy);
        throw ex;
    }
    va_end(argsCopy);
}


} // namespace elasticlient
