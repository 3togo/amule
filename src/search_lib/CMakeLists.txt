# Search Library CMakeLists.txt
# Provides a modular, testable search functionality independent of GUI

add_library(search_core
    SearchController.cpp
    AMuleSearchEngine.cpp
)

# Create an interface library for headers only
add_library(search_interface INTERFACE)
target_include_directories(search_interface
    INTERFACE .
)

target_link_libraries(search_core
    PRIVATE mulecommon
)

target_include_directories(search_core
    PUBLIC .
    PRIVATE ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/src/libs
    PRIVATE ${CMAKE_SOURCE_DIR}/src/libs/common
    PRIVATE ${CMAKE_SOURCE_DIR}/src/include
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../..
)