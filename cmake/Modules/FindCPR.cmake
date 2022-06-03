# - C++ Requests, Curl for People. Copied from whoshuu/cpr (MIT license)
# This module is a libcurl wrapper written in modern C++.
# It provides an easy, intuitive, and efficient interface to
# a host of networking methods.
#
# Finding this module will define the following variables:
#  CPR_FOUND - True if the core library has been found
#  CPR_LIBRARIES - Path to the core library archive
#  CPR_INCLUDE_DIRS - Path to the include directories. Gives access
#                     to cpr.h, which must be included in every
#                     file that uses this interface

include(FindPackageHandleStandardArgs)

# Checks an environment variable; note that the first check
# does not require the usual CMake $-sign.
IF( DEFINED ENV{CPR_DIR} )
  SET( CPR_DIR "$ENV{CPR_DIR}" )
ENDIF()

find_path(CPR_INCLUDE_DIR
          NAMES cpr.h cpr/cpr.h
          NO_DEFAULT_PATH
          HINTS ${CPR_DIR}
          PATH_SUFFIXES include)

find_library(CPR_LIBRARY
             NAMES cpr
             NO_DEFAULT_PATH
             HINTS ${CPR_LIBRARY_ROOT}
                   ${CPR_DIR}
             PATH_SUFFIXES lib)

find_package_handle_standard_args(CPR REQUIRED_VARS CPR_LIBRARY CPR_INCLUDE_DIR)
mark_as_advanced(CPR_LIBRARY CPR_INCLUDE_DIR)

if(CPR_FOUND)
    set(CPR_LIBRARIES ${CPR_LIBRARY} CACHE INTERNAL "")
    set(CPR_INCLUDE_DIRS ${CPR_INCLUDE_DIR} CACHE INTERNAL "")
endif()
