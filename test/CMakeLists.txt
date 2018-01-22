include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/../src  # Let test to access internal stuff.
                    ${ELASTICLIENT_INCLUDE_DIRS}
                    ${HTTPMOCKSERVER_INCLUDE_DIRS}
                    ${GTEST_INCLUDE_DIRS})

add_executable(tests-elasticlient
               tests-elasticlient.cc)


target_link_libraries(tests-elasticlient
                      ${ELASTICLIENT_LIBRARIES}
                      ${HTTPMOCKSERVER_LIBRARIES}
                      ${GTEST_LIBRARIES}
                      -lpthread)

add_test(NAME tests-elasticlient COMMAND tests-elasticlient)
