include_directories(${ELASTICLIENT_INCLUDE_DIRS}
                    ${CPR_INCLUDE_DIRS}
                    ${RapidJSON_INCLUDE_DIRS})

add_library(${ELASTICLIENT_LIBRARY}
            client.cc
            bulk.cc
            scroll.cc
            logging.cc

            "${ELASTICLIENT_INCLUDE_DIR}/elasticlient/client.h"
            "${ELASTICLIENT_INCLUDE_DIR}/elasticlient/logging.h"
            "${ELASTICLIENT_INCLUDE_DIR}/elasticlient/bulk.h"
            "${ELASTICLIENT_INCLUDE_DIR}/elasticlient/scroll.h")

if(BUILD_SHARED_LIBS)
    set_target_properties(${ELASTICLIENT_LIBRARY}
                          PROPERTIES
                              VERSION "${ELASTICLIENT_VERSION_MAJOR}.${ELASTICLIENT_VERSION_MINOR}.${ELASTICLIENT_VERSION_PATCH}"
                              SOVERSION ${ELASTICLIENT_VERSION_MAJOR})
endif()

target_link_libraries(${ELASTICLIENT_LIBRARY}
                      ${CPR_LIBRARIES})

install(TARGETS ${ELASTICLIENT_LIBRARY} LIBRARY
        DESTINATION lib)
