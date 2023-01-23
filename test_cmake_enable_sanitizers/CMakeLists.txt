
# Test stuff in cmake/

cmake_minimum_required(VERSION 3.22)
project(test_rhmiscutil LANGUAGES CXX)

message(STATUS "CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

include(../cmake/add_build_types.cmake)


include(../cmake/add_stack_protection_options.cmake)

include(CTest)

add_executable(testSanitizers testSanitizers.cpp)
add_test(NAME testSanitizers COMMAND testSanitizers)

add_executable(testThreadSanitizer testThreadSanitizer.cpp)
add_test(NAME testThreadSanitizer COMMAND testThreadSanitizer)
target_link_libraries(testThreadSanitizer PRIVATE pthread)

add_executable(testStackProtectionOptions testStackProtectionOptions.cpp)
add_test(NAME testStackProtectionOptions COMMAND testStackProtectionOptions)

