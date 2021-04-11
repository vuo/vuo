cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(CompileAndRunInNewProcess)

set(CMAKE_FRAMEWORK_PATH /Library/Developer/Vuo/framework CACHE STRING "")
find_library(VUO_FRAMEWORK Vuo)

add_executable(CompileAndRunInNewProcess CompileAndRunInNewProcess.cc)
target_compile_features(CompileAndRunInNewProcess PRIVATE cxx_std_11)
target_compile_definitions(CompileAndRunInNewProcess PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_directories(CompileAndRunInNewProcess PRIVATE ${VUO_FRAMEWORK}/Modules)
target_link_libraries(CompileAndRunInNewProcess ${VUO_FRAMEWORK})
