if(BUILD_SHARED_LIBS)
    set(ELASTICLIENT_STATICLIB NO)
else()
    set(ELASTICLIENT_STATICLIB YES)
endif()

include_directories(${ELASTICLIENT_INCLUDE_DIRS}
                    ${CPR_INCLUDE_DIRS}
                    ${JSONCPP_INCLUDE_DIRS})

add_library(${ELASTICLIENT_LIBRARY}
            client.cc
            bulk.cc
            scroll.cc
            logging.cc

            "${ELASTICLIENT_INCLUDE_DIR}/elasticlient/client.h"
            "${ELASTICLIENT_INCLUDE_DIR}/elasticlient/logging.h"
            "${ELASTICLIENT_INCLUDE_DIR}/elasticlient/bulk.h"
            "${ELASTICLIENT_INCLUDE_DIR}/elasticlient/scroll.h")

set_target_properties(${ELASTICLIENT_LIBRARY}
                      PROPERTIES
                          VERSION "${ELASTICLIENT_VERSION_MAJOR}.${ELASTICLIENT_VERSION_MINOR}.${ELASTICLIENT_VERSION_PATCH}"
                          SOVERSION ${ELASTICLIENT_VERSION_MAJOR}
                          INTERFACE_COMPILE_DEFINITIONS ELASTICLIENT_STATICLIB)

target_link_libraries(${ELASTICLIENT_LIBRARY}
                      ${JSONCPP_LIBRARIES}
                      ${CPR_LIBRARIES})

install(TARGETS ${ELASTICLIENT_LIBRARY} LIBRARY
        DESTINATION lib)
