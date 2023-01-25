
# The t1 cmake library
# get all files according to pattern, relative to TESTROOT
macro(find_test_sources OUT DIR TESTROOT PATTERN)
    file(GLOB_RECURSE ${OUT} RELATIVE "${TESTROOT}" "${DIR}/${PATTERN}")
endmacro()

macro(split_main_path MAIN NAME PATH)
    get_filename_component(${NAME} "${MAIN}" NAME_WLE)
    get_filename_component(${PATH} "${MAIN}" DIRECTORY)
endmacro()

macro(add_test MAIN)
    set(_OPTIONS)
    set(_SINGLE_VAL_ARGS)
    set(_MULTI_VAL_ARGS INCLUDE_DIRS LIBRARIES SOURCE_DEPS)

    cmake_parse_arguments(ADD_TEST "${_OPTIONS}" "${_SINGLE_VAL_ARGS}" "${_MULTI_VAL_ARGS}" ${ARGN})

    split_main_path(${MAIN} TEST_NAME_ TEST_PATH_)
    set(TEST_OUTPUT_DIR_ "${CMAKE_BINARY_DIR}/${TEST_PATH_}")
    add_executable(${TEST_NAME_} ${MAIN} ${ADD_TEST_SOURCE_DEPS})

    if (DEFINED ADD_TEST_INCLUDE_DIRS)
        message(${ADD_TEST_INCLUDE_DIRS})
        target_include_directories(${TEST_NAME_} PRIVATE ${ADD_TEST_INCLUDE_DIRS})
    endif()

    if (DEFINED ADD_TEST_LIBRARIES)
        target_link_libraries(${TEST_NAME_} ${ADD_TEST_LIBRARIES})
    endif()

    file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR_}")
    set_target_properties(${TEST_NAME_} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${TEST_OUTPUT_DIR_}")

    set(T1_TEST_TARGETS "${T1_TEST_TARGETS}" "${TEST_NAME_}")
    set(T1_TEST_EXECUTABLES "${T1_TEST_EXECUTABLES}" "${TEST_OUTPUT_DIR_}/${TEST_NAME_}")
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

# default macro to add all .cpp files in a directory; use only if possible
# also adds all non-main sources of optional arguments as dependencies,
# can get big quickly.
# defines TEST_SOURCES
macro(add_test_directory DIR)
    set(_OPTIONS)
    set(_SINGLE_VAL_ARGS)
    set(_MULTI_VAL_ARGS INCLUDE_DIRS LIBRARIES SOURCE_DEPS)

    cmake_parse_arguments(ADD_TEST_DIRECTORY "${_OPTIONS}" "${_SINGLE_VAL_ARGS}" "${_MULTI_VAL_ARGS}" ${ARGN})

    message("-- t1: adding test directory ${DIR}")
    find_test_sources(TEST_SOURCES "${DIR}" "${CMAKE_CURRENT_LIST_DIR}" "*.cpp")
    find_test_non_main_source_deps(TEST_DEPS_ ${ADD_TEST_DIRECTORY_SOURCE_DEPS})
    
    foreach(INPUT_FILE ${TEST_SOURCES})
        add_test("${INPUT_FILE}" INCLUDE_DIRS ${ADD_TEST_DIRECTORY_INCLUDE_DIRS} LIBRARIES ${ADD_TEST_DIRECTORY_LIBRARIES} SOURCE_DEPS ${TEST_DEPS_})
    endforeach()
endmacro()

# adds a command to build all tests and a command to run all tests
macro(register_tests)
    add_custom_target(tests)

    foreach(TEST ${T1_TEST_TARGETS})
        add_dependencies(tests "${TEST}")
    endforeach()

    add_custom_target(runtests)
    add_custom_target(vruntests)
    add_custom_target(valgrindtests)

    foreach(EXE ${T1_TEST_EXECUTABLES})
        split_main_path(${EXE} TEST_NAME_ TEST_PATH_)
        add_custom_target("run${TEST_NAME_}" COMMAND "${EXE}")
        add_dependencies(runtests "run${TEST_NAME_}")

        add_custom_target("vrun${TEST_NAME_}" COMMAND "${EXE}" "-v")
        add_dependencies(vruntests "vrun${TEST_NAME_}")

        add_custom_target("valgrind${TEST_NAME_}" COMMAND "valgrind" "--leak-check=full" "--error-exitcode=1" "--log-file=${TEST_PATH_}/valgrind.log" ${ARGN} "${EXE}")
        add_dependencies(valgrindtests "valgrind${TEST_NAME_}")
    endforeach()
endmacro()
