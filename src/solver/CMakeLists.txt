find_library(GECODE_SEARCH gecodesearch)
find_library(GECODE_INT gecodeint)
find_library(GECODE_KERNEL gecodekernel)
find_library(GECODE_SUPPORT gecodesupport)

add_library(Solver solver.cpp globals.hpp solver.hpp)

target_link_libraries(Solver PUBLIC
    "${GECODE_SEARCH}"
    "${GECODE_INT}"
    "${GECODE_KERNEL}"
    "${GECODE_SUPPORT}")