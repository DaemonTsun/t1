cmake_minimum_required(VERSION 3.18)

project(t1)

# sources
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(SOURCE_FILE "${SOURCE_DIR}/t1.hpp")
set(SOURCE_CMAKE_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/t1Config.cmake")

# settings
set(CMAKE_CXX_STANDARD 17)

# tests
find_package(t1)
if (NOT DEFINED t1_DIR OR t1_DIR)
    add_test_directory("${CMAKE_SOURCE_DIR}/tests")
    register_tests()
endif()

# install
# t1 is a header-only library
install(FILES "${SOURCE_FILE}" DESTINATION "include/${PROJECT_NAME}")
install(FILES "${SOURCE_CMAKE_CONFIG_FILE}" DESTINATION "share/${PROJECT_NAME}/cmake")
