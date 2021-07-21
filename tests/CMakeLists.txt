project(test)

link_libraries(pthread)

find_package(GTest)
link_libraries(GTest::GTest GTest::Main)

link_libraries(disruptor)

aux_source_directory(. TEST_SOURCE)
add_executable(${PROJECT_NAME} ${TEST_SOURCE})
