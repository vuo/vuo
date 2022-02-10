cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
cmake_policy(VERSION 3.16...3.20)

project(RunImageFilterQt)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(CMAKE_FRAMEWORK_PATH /Library/Developer/Vuo/framework CACHE STRING "")
find_library(VUO_FRAMEWORK Vuo)

set(CMAKE_AUTOMOC ON)
add_executable(RunImageFilter-Qt
	GLWidget.cc
	GLWidget.hh
	main.cc
)
target_compile_features(RunImageFilter-Qt PRIVATE cxx_std_11)
target_link_directories(RunImageFilter-Qt PRIVATE ${VUO_FRAMEWORK}/Modules)
target_link_libraries(RunImageFilter-Qt
	"-framework OpenGL"
	Qt5::Widgets
	${VUO_FRAMEWORK}
)
