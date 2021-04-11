cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(CompileAndRunInCurrentProcess)

set(CMAKE_FRAMEWORK_PATH /Library/Developer/Vuo/framework CACHE STRING "")
find_library(VUO_FRAMEWORK Vuo)

add_executable(CompileAndRunInCurrentProcess CompileAndRunInCurrentProcess.cc)
target_compile_features(CompileAndRunInCurrentProcess PRIVATE cxx_std_11)
target_compile_definitions(CompileAndRunInCurrentProcess PRIVATE SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_directories(CompileAndRunInCurrentProcess PRIVATE ${VUO_FRAMEWORK}/Modules)
target_link_libraries(CompileAndRunInCurrentProcess ${VUO_FRAMEWORK})
