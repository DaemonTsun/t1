
# The t1 cmake library
# get all files according to pattern, relative to TESTROOT
macro(find_test_sources OUT DIR TESTROOT PATTERN)
    file(GLOB_RECURSE ${OUT} RELATIVE "${TESTROOT}" "${DIR}/${PATTERN}")
endmacro()

# these are mostly relevant for Windows
set(_t1Config_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
cmake_path(GET _t1Config_CMAKE_DIR PARENT_PATH _t1_SHARE_DIR)
cmake_path(GET _t1_SHARE_DIR PARENT_PATH _t1_SHARE_PARENT_DIR)
cmake_path(GET _t1_SHARE_PARENT_DIR PARENT_PATH _t1_INSTALL_DIR)
set(_t1_INCLUDE_DIR "${_t1_INSTALL_DIR}/include")

macro(split_path_into_filename_and_parent_path FULLPATH NAME PATH)
    get_filename_component(${NAME} "${FULLPATH}" NAME_WLE)
    get_filename_component(${PATH} "${FULLPATH}" DIRECTORY)
endmacro()

macro(add_test TEST_SRC_FILE)
    set(_OPTIONS)
    set(_SINGLE_VAL_ARGS CPP_VERSION)
    set(_MULTI_VAL_ARGS INCLUDE_DIRS LIBRARIES SOURCE_DEPS CPP_WARNINGS)

    message(VERBOSE "t1: adding test ${TEST_SRC_FILE}")

    cmake_parse_arguments(ADD_TEST "${_OPTIONS}" "${_SINGLE_VAL_ARGS}" "${_MULTI_VAL_ARGS}" ${ARGN})

    split_path_into_filename_and_parent_path(${TEST_SRC_FILE} TEST_NAME_ TEST_PATH_)
    set(TEST_OUTPUT_DIR_ "${CMAKE_CURRENT_BINARY_DIR}/${TEST_PATH_}")

    if (NOT TARGET "${TEST_NAME_}")
        add_executable(${TEST_NAME_})

        target_sources(${TEST_NAME_} PRIVATE ${TEST_SRC_FILE} ${ADD_TEST_SOURCE_DEPS})

        if (NOT DEFINED ADD_TEST_CPP_VERSION)
            set(ADD_TEST_CPP_VERSION 20)
        endif()

        if (DEFINED ADD_TEST_INCLUDE_DIRS)
            target_include_directories(${TEST_NAME_} PRIVATE ${ADD_TEST_INCLUDE_DIRS})
        endif()

        if (_t1_INCLUDE_DIR AND EXISTS "${_t1_INCLUDE_DIR}")
            target_include_directories(${TEST_NAME_} PRIVATE "${_t1_INCLUDE_DIR}")
        endif()

        if (DEFINED ADD_TEST_LIBRARIES)
            target_link_libraries(${TEST_NAME_} ${ADD_TEST_LIBRARIES})
        endif()

        if (DEFINED ADD_TEST_CPP_WARNINGS)
            target_compile_options(${TEST_NAME_} PRIVATE ${ADD_TEST_CPP_WARNINGS})
        endif()

        file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR_}")
        set_target_properties("${TEST_NAME_}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_OUTPUT_DIR_}")
        set_property(TARGET "${TEST_NAME_}" PROPERTY CXX_STANDARD ${ADD_TEST_CPP_VERSION})

        set(T1_TEST_TARGETS "${T1_TEST_TARGETS}" "${TEST_NAME_}")
        set(T1_TEST_EXECUTABLES "${T1_TEST_EXECUTABLES}" "${TEST_OUTPUT_DIR_}/${TEST_NAME_}")
    else()
        message(WARNING "t1: test with name ${TEST_NAME_} already registered, skipping.")
    endif()
endmacro()

macro(find_test_non_main_source_deps TEST_DEPS_)
    set(SRCS__ ${ARGN})
    
    set(${TEST_DEPS_})
    foreach(SRC ${SRCS__})
        file(GLOB_RECURSE SRC__ "${SRC}/*.c" "${SRC}/*.cpp")
        
        foreach(f ${SRC__})
            if(NOT f MATCHES ".*main.c(pp)?$")
                list(APPEND ${TEST_DEPS_} ${f})
            endif()
        endforeach()
    endforeach()
endmacro()

# default macro to add all .cpp files in a directory; use only if possible.
# also adds all non-main sources of optional arguments as dependencies,
# can get big quickly.
# defines TEST_SOURCES
macro(add_test_directory DIR)
    set(_OPTIONS)
    set(_SINGLE_VAL_ARGS CPP_VERSION)
    set(_MULTI_VAL_ARGS INCLUDE_DIRS LIBRARIES SOURCE_DEPS CPP_WARNINGS)

    cmake_parse_arguments(ADD_TEST_DIRECTORY "${_OPTIONS}" "${_SINGLE_VAL_ARGS}" "${_MULTI_VAL_ARGS}" ${ARGN})

    if (NOT DEFINED ADD_TEST_DIRECTORY_CPP_VERSION)
        set(ADD_TEST_DIRECTORY_CPP_VERSION 20)
    endif()

    message(STATUS "t1: adding test directory ${DIR}")
    find_test_sources(TEST_SOURCES "${DIR}" "${CMAKE_CURRENT_LIST_DIR}" "*.cpp")
    find_test_non_main_source_deps(TEST_DEPS_ ${ADD_TEST_DIRECTORY_SOURCE_DEPS})
    
    foreach(INPUT_FILE ${TEST_SOURCES})
        add_test("${INPUT_FILE}"
            CPP_VERSION ${ADD_TEST_DIRECTORY_CPP_VERSION}
            CPP_WARNINGS ${ADD_TEST_DIRECTORY_CPP_WARNINGS}
            INCLUDE_DIRS ${ADD_TEST_DIRECTORY_INCLUDE_DIRS}
            LIBRARIES ${ADD_TEST_DIRECTORY_LIBRARIES}
            SOURCE_DEPS ${TEST_DEPS_})
    endforeach()
endmacro()

# adds a command to build all tests and a command to run all tests
macro(register_tests)
    if (NOT TARGET tests)
        add_custom_target(tests)
    endif()

    foreach(TEST ${T1_TEST_TARGETS})
        add_dependencies(tests "${TEST}")
    endforeach()

    if (NOT TARGET runtests)
        add_custom_target(runtests)
    endif()

    if (NOT TARGET vruntests)
        add_custom_target(vruntests)
    endif()

    if (NOT TARGET valgrindtests)
        add_custom_target(valgrindtests)
    endif()

    foreach(EXE ${T1_TEST_EXECUTABLES})
        split_path_into_filename_and_parent_path(${EXE} TEST_NAME_ TEST_PATH_)

        if (NOT TARGET "run${TEST_NAME_}")
            add_custom_target("run${TEST_NAME_}" COMMAND "${EXE}")
            add_dependencies(runtests "run${TEST_NAME_}")

            add_custom_target("vrun${TEST_NAME_}" COMMAND "${EXE}" "-v")
            add_dependencies(vruntests "vrun${TEST_NAME_}")

            add_custom_target("valgrind${TEST_NAME_}" COMMAND "valgrind" "--leak-check=full" "--error-exitcode=1" "--log-file=${TEST_PATH_}/valgrind.log" ${ARGN} "${EXE}")
            add_dependencies(valgrindtests "valgrind${TEST_NAME_}")
        else()
            message(WARNING "t1: test with name ${TEST_NAME_} already registered, skipping.")
        endif()

    endforeach()
endmacro()
