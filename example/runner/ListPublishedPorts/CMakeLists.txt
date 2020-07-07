cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(ListPublishedPorts)

set(CMAKE_FRAMEWORK_PATH
	../../../framework
	../../../build/lib
)
find_library(VUO_FRAMEWORK Vuo)

add_executable(ListPublishedPorts ListPublishedPorts.cc)
target_compile_features(ListPublishedPorts PRIVATE cxx_std_11)
target_compile_definitions(ListPublishedPorts PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_directories(ListPublishedPorts PRIVATE ${VUO_FRAMEWORK}/Modules)
target_link_libraries(ListPublishedPorts ${VUO_FRAMEWORK})
