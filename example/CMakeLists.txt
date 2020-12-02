include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${ELASTICLIENT_INCLUDE_DIRS})

add_executable(hello-world
               hello-world.cc)

target_link_libraries(hello-world
                      ${ELASTICLIENT_LIBRARIES})

add_executable(initializations
               initializations.cc)

target_link_libraries(initializations
                      ${ELASTICLIENT_LIBRARIES})
