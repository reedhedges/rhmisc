
# Test stuff in cmake/

cmake_minimum_required(VERSION 3.22)
project(test_rhmiscutil LANGUAGES CXX)

message(STATUS "chartplotter: CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
message(STATUS "chartplotter: CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}")

include(../cmake/add_build_types.cmake)
add_build_types()

include(CTest)
add_executable(testSanitizers testSanitizers.cpp)
add_test(NAME testSanitizers COMMAND testSanitizers)


