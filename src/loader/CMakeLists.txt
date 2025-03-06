add_library(Loader INTERFACE loader.hpp)

target_include_directories(Loader INTERFACE 
    "${PROJECT_SOURCE_DIR}/libs/json"
    "${PROJECT_SOURCE_DIR}/libs/mongo-cxx-driver/include"
    "${PROJECT_SOURCE_DIR}/src/solver")
