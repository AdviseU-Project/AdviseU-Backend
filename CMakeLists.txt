cmake_minimum_required(VERSION 3.22.1)

project(AdviseU)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
find_package(Boost 1.88.0 REQUIRED COMPONENTS asio beast)

pkg_check_modules(mongocxx REQUIRED IMPORTED_TARGET libmongocxx)

add_executable(AdviseU main.cpp shims.hpp)

add_subdirectory(json)
add_subdirectory(loader)
add_subdirectory(solver)

target_link_libraries(AdviseU 
    PUBLIC
        Solver
        Boost::asio 
        Boost::beast
        PkgConfig::mongocxx
    INTERFACE
        Loader
        Json)

target_include_directories(AdviseU PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/json"
    "${PROJECT_SOURCE_DIR}/solver"
    "${PROJECT_SOURCE_DIR}/loader")