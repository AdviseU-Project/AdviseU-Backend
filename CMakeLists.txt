cmake_minimum_required(VERSION 3.22.1)

project(AdviseU-Backend)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
find_package(Boost 1.88.0 REQUIRED COMPONENTS asio beast)

pkg_check_modules(mongocxx REQUIRED IMPORTED_TARGET libmongocxx)

add_executable(AdviseU-Backend src/main.cpp src/shims.hpp)

add_subdirectory(src/solver)
add_subdirectory(src/loader)
add_subdirectory(libs/json)

target_link_libraries(AdviseU-Backend
    PUBLIC
        Solver
        Boost::asio 
        Boost::beast
        PkgConfig::mongocxx
    INTERFACE
        Loader
        Json
)

target_include_directories(AdviseU-Backend PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/src/solver"
    "${PROJECT_SOURCE_DIR}/src/loader"
    "${PROJECT_SOURCE_DIR}/libs/json"
)
