cmake_minimum_required(VERSION 3.7)

project(evp)

add_executable(evp main.cpp config.cpp)

set_target_properties(evp PROPERTIES CXX_STANDARD 20)

find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(evp PRIVATE JsonCpp::JsonCpp)

find_package(yaml-cpp CONFIG REQUIRED)
target_link_libraries(evp PRIVATE yaml-cpp)

find_package(argparse CONFIG REQUIRED)
target_link_libraries(evp PRIVATE argparse::argparse)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(evp PRIVATE fmt::fmt-header-only)

install(TARGETS evp)
