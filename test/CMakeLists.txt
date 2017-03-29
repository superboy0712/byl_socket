project(bylSocket_test)
enable_testing()

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})


aux_source_directory(. TESTSRC)
add_executable(alltest ${TESTSRC})
message(STATUS "TESTSRC is ${TESTSRC}")
target_link_libraries(alltest
        gmock_main
        gmock
        ${CMAKE_THREAD_LIBS_INIT}
        dynamic_bylSocket)

message(STATUS "CMAKE_THREAD_LIBS_INIT is ${CMAKE_THREAD_LIBS_INIT}")
message(STATUS "GTEST_BOTH_LIBRARIES is ${GTEST_BOTH_LIBRARIES}")

add_test(TestsInalltest alltest)