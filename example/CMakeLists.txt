include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${ELASTICLIENT_INCLUDE_DIRS}
                    ${CPR_INCLUDE_DIRS}
                    ${JSONCPP_INCLUDE_DIRS})

add_executable(hello-world
               hello-world.cc)

target_link_libraries(hello-world
                      ${ELASTICLIENT_LIBRARIES})
