cmake_minimum_required(VERSION 3.22.1)

project(AdviseU-Backend)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(AdviseU-Backend src/main.cpp src/shims.hpp)

add_subdirectory(src/solver)
add_subdirectory(src/loader)
add_subdirectory(libs/json)

target_link_libraries(AdviseU-Backend
    PUBLIC
        Solver
    INTERFACE
        Loader
        Json
)

target_include_directories(AdviseU-Backend PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/src/solver"
    "${PROJECT_SOURCE_DIR}/src/loader"
    "${PROJECT_SOURCE_DIR}/libs/json"
    "${PROJECT_SOURCE_DIR}/libs/boost/asio"
    "${PROJECT_SOURCE_DIR}/libs/boost/beast"
    "${PROJECT_SOURCE_DIR}/libs/mongo-cxx-driver/include"
)

target_link_libraries(AdviseU-Backend PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/mongo-cxx-driver/lib/libmongocxx.so"
    "${PROJECT_SOURCE_DIR}/libs/mongo-cxx-driver/lib/libbsoncxx.so"
)

