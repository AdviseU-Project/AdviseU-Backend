set(GECODE_ROOT "${CMAKE_SOURCE_DIR}/libs/gecode-release-6.2.0/install")

include_directories("${GECODE_ROOT}/include")
link_directories("${GECODE_ROOT}/lib")

find_library(GECODE_SEARCH gecodesearch PATHS "${GECODE_ROOT}/lib" NO_DEFAULT_PATH)
find_library(GECODE_INT gecodeint PATHS "${GECODE_ROOT}/lib" NO_DEFAULT_PATH)
find_library(GECODE_KERNEL gecodekernel PATHS "${GECODE_ROOT}/lib" NO_DEFAULT_PATH)
find_library(GECODE_SUPPORT gecodesupport PATHS "${GECODE_ROOT}/lib" NO_DEFAULT_PATH)

message(STATUS "GECODE_SEARCH found at: ${GECODE_SEARCH}")
message(STATUS "GECODE_INT found at: ${GECODE_INT}")
message(STATUS "GECODE_KERNEL found at: ${GECODE_KERNEL}")
message(STATUS "GECODE_SUPPORT found at: ${GECODE_SUPPORT}")

add_library(Solver solver.cpp globals.hpp solver.hpp)

target_link_libraries(Solver PUBLIC
    "${GECODE_SEARCH}"
    "${GECODE_INT}"
    "${GECODE_KERNEL}"
    "${GECODE_SUPPORT}"
)

set_target_properties(Solver PROPERTIES
    BUILD_RPATH "${GECODE_ROOT}/lib"
)
