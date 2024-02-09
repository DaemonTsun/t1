cmake_minimum_required(VERSION 3.18)

# Version 3:
# remove C++ standard library, remove exceptions, make library simpler
# add time display for all test files (and units using --verbose)

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
        set(CMAKE_CXX_STANDARD 20)

        include_directories("${SOURCE_DIR}")

        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            add_compile_options(-fno-exceptions -fno-rtti)
        elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
            add_compile_options(/GR- /EHsc)
            add_compile_definitions(_HAS_EXCEPTIONS=0)
            add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
            add_compile_definitions(WIN32_LEAN_AND_MEAN=1)
            endif()

        add_test_directory("${CMAKE_CURRENT_SOURCE_DIR}/tests" INCLUDE_DIRS "${SOURCE_DIR}")
        register_tests()
    endif()
endif()


# install
# t1 is a header-only library
install(FILES "${SOURCE_FILE}" DESTINATION "include/${PROJECT_NAME}")
install(FILES "${SOURCE_CMAKE_CONFIG_FILE}" DESTINATION "share/${PROJECT_NAME}/cmake")
