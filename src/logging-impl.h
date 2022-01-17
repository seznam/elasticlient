/**
 * \file
 * Internal implementation of for log messages.
 */

#pragma once

#include "elasticlient/logging.h"
#include <fmt/core.h>

#define LOG(logLevel, args...)                                    \
    do {                                                          \
        elasticlient::log(logLevel, fmt::format(args));           \
    } while (false)


namespace elasticlient {

/// Will call specified log callback function with \p logLevel and \p message
void log(LogLevel logLevel, const std::string &message);

} // namespace elasticlient
