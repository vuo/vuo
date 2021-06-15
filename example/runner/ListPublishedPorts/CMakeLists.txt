cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
cmake_policy(VERSION 3.16...3.20)

project(ListPublishedPorts)

set(CMAKE_FRAMEWORK_PATH /Library/Developer/Vuo/framework CACHE STRING "")
find_library(VUO_FRAMEWORK Vuo)

add_executable(ListPublishedPorts ListPublishedPorts.cc)
target_compile_features(ListPublishedPorts PRIVATE cxx_std_11)
target_compile_definitions(ListPublishedPorts PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_directories(ListPublishedPorts PRIVATE ${VUO_FRAMEWORK}/Modules)
target_link_libraries(ListPublishedPorts ${VUO_FRAMEWORK})
