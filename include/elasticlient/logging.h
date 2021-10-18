/**
 * \file
 * Module for maintain log messages.
 */

#pragma once
#include <string>

#ifndef __GNUC__ 
#undef ERROR
#endif

/// The elasticlient namespace
namespace elasticlient {


/// Levels of logging
enum class LogLevel {
    FATAL   = 0,
    ERROR   = 1,
    WARNING = 2,
    INFO    = 3,
    DEBUG   = 4
};


/// Definition of LogCallback
using LogCallback = void(*)(LogLevel, const std::string &);


/**
 * Function for set custom logging callback. If logging is wanted it is recomended to set custom
 * callback before using any of elasticlient class. Also this function should be called before
 * forking your program in more threads.
 * If custom LogCallback function is not set logging is disabled.
 */
void setLogFunction(LogCallback extLogFunction);


} // namespace elasticlient
