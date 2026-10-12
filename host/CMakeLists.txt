cmake_minimum_required(VERSION 3.20)
project(ps3link_host CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

add_library(ps3link STATIC src/client.cpp src/mock_server.cpp)
target_include_directories(ps3link PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common src)
find_package(Threads REQUIRED)
target_link_libraries(ps3link PUBLIC Threads::Threads)

add_executable(ps3sh src/ps3sh.cpp)
target_link_libraries(ps3sh PRIVATE ps3link)

add_executable(ps3mock src/ps3mock.cpp)
target_link_libraries(ps3mock PRIVATE ps3link)

enable_testing()
add_executable(test_ps3link tests/test_ps3link.cpp)
target_link_libraries(test_ps3link PRIVATE ps3link)
add_test(NAME ps3link COMMAND test_ps3link)
