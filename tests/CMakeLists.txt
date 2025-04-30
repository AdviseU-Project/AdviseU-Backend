include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.16.0
)

FetchContent_GetProperties(googletest)

if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

mark_as_advanced(
    BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

set_target_properties(gtest PROPERTIES FOLDER extern)
set_target_properties(gtest_main PROPERTIES FOLDER extern)
set_target_properties(gmock PROPERTIES FOLDER extern)
set_target_properties(gmock_main PROPERTIES FOLDER extern)

add_executable(
    degree_plan_test
    degree_plan_test.cpp
)

target_link_libraries(degree_plan_test
    PUBLIC
        Solver
        PkgConfig::mongocxx
        gtest
    INTERFACE
        Loader
)

target_include_directories(degree_plan_test PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/src/solver"
    "${PROJECT_SOURCE_DIR}/src/loader"
    "${PROJECT_SOURCE_DIR}/libs/json"
)

gtest_discover_tests(degree_plan_test degree_plan_test 
    DISCOVERY_MODE PRE_TEST
    DISCOVERY_TIMEOUT 240
)