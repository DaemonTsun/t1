cmake_minimum_required(VERSION 3.18)

# Version 2:
# buildless installation, ctest compatibility.

# You can skip trying to build the tests (e.g. if there's no compiler or
# toolchain set in the environment) and only install t1 by running:
#
#    cmake -Donly_install=1 <dir of this CMakeLists.txt file> -B <output dir>
#    cmake --install <output dir>

set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SOURCE_FILE "${SOURCE_DIR}/t1/t1.hpp")
set(SOURCE_CMAKE_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/t1Config.cmake")

if (only_install)
    project(t1 LANGUAGES NONE)
else()
    project(t1)

    # tests
    find_package(t1 REQUIRED NO_DEFAULT_PATH PATHS ./cmake/)

    if (NOT DEFINED t1_DIR OR t1_DIR)
        # settings
        set(CMAKE_CXX_STANDARD 17)

        include_directories("${SOURCE_DIR}")

        add_test_directory("${CMAKE_CURRENT_SOURCE_DIR}/tests" INCLUDE_DIRS "${SOURCE_DIR}")
        register_tests()
    endif()
endif()


# install
# t1 is a header-only library
install(FILES "${SOURCE_FILE}" DESTINATION "include/${PROJECT_NAME}")
install(FILES "${SOURCE_CMAKE_CONFIG_FILE}" DESTINATION "share/${PROJECT_NAME}/cmake")
