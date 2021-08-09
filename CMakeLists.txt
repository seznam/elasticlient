cmake_minimum_required(VERSION 2.8.7)
project(Elasticlient LANGUAGES CXX)

message(STATUS "Checking compiler C++11 support.")
if(CMAKE_VERSION VERSION_LESS "3.1")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
    endif()
else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -Wall -Wextra -Wfatal-errors -Wformat-security -Wuninitialized -Wfloat-equal -fPIC -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

macro(get_variable VAR_NAME VAR_TEXT DEFAULT_VALUE IS_OPTION)
    if(${IS_OPTION})
        option(${VAR_NAME} ${VAR_TEXT} ${DEFAULT_VALUE})
    elseif(NOT DEFINED ${VAR_NAME})
        set(${VAR_NAME} ${DEFAULT_VALUE})
    endif()
    # Also check if defined in environment
    if(DEFINED ENV{${VAR_NAME}})
        set(${VAR_NAME} $ENV{${VAR_NAME}})
    endif()
    message(STATUS "  ${VAR_NAME}: ${${VAR_NAME}}")
endmacro()

get_variable(ELASTICLIENT_VERSION_MAJOR "Set elasticlient major version." 2 NO)
get_variable(ELASTICLIENT_VERSION_MINOR "Set elasticlient minor version." 1 NO)
get_variable(ELASTICLIENT_VERSION_PATCH "Set elasticlient patch version." 0 NO)
get_variable(BUILD_ELASTICLIENT_TESTS "Build tests for elasticlient library." YES YES)
get_variable(BUILD_ELASTICLIENT_EXAMPLE "Build exmaple program which using elasticlient library." YES YES)
get_variable(BUILD_SHARED_LIBS "Build shared libraries" YES YES)

get_variable(USE_ALL_SYSTEM_LIBS "Will found all libraries in system." NO YES)
if(USE_ALL_SYSTEM_LIBS)
    set(USE_SYSTEM_RAPIDJSON YES)
    set(USE_SYSTEM_CPR YES)
    set(USE_SYSTEM_HTTPMOCKSERVER YES)
    set(USE_SYSTEM_GTEST YES)
else()
    get_variable(USE_SYSTEM_RAPIDJSON "Will found RapidJson library in system." NO YES)
    get_variable(USE_SYSTEM_CPR "Will found CPR library in system." NO YES)
    get_variable(USE_SYSTEM_HTTPMOCKSERVER "Will found HTTPMockServer library in system." NO YES)
    get_variable(USE_SYSTEM_GTEST "Will found GTest library in system." NO YES)
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
link_directories(${LIBRARY_OUTPUT_PATH})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")
message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

# Dependencies are solved in external subdirectory
add_subdirectory(external)

set(ELASTICLIENT_LIBRARY elasticlient CACHE INTERNAL "")
set(ELASTICLIENT_LIBRARIES ${ELASTICLIENT_LIBRARY} ${CPR_LIBRARIES} CACHE INTERNAL "")
set(ELASTICLIENT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "")
set(ELASTICLIENT_INCLUDE_DIRS ${ELASTICLIENT_INCLUDE_DIR} ${CPR_INCLUDE_DIRS} ${RapidJSON_INCLUDE_DIRS} CACHE INTERNAL "")

add_subdirectory(src)

if(BUILD_ELASTICLIENT_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_ELASTICLIENT_EXAMPLE)
    add_subdirectory(example)
endif()

install(DIRECTORY ${ELASTICLIENT_INCLUDE_DIR}/elasticlient
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
