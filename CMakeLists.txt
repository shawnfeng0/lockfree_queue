project(disruptor)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 11)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

link_libraries(pthread)

add_executable(test tests/test_one_publisher_one_subscriber.cc)
target_link_libraries(test PRIVATE ${PROJECT_NAME})
find_package(GTest)
target_link_libraries(test PRIVATE GTest::GTest GTest::Main)
