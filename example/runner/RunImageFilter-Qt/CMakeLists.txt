cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(CMAKE_FRAMEWORK_PATH ../../../framework)
find_library(VUO_FRAMEWORK Vuo)

set(CMAKE_AUTOMOC ON)
add_executable(RunImageFilter-Qt
	GLWidget.cc
	main.cc
)
target_compile_features(RunImageFilter-Qt PRIVATE cxx_std_11)
target_link_directories(RunImageFilter-Qt PRIVATE ${VUO_FRAMEWORK}/Modules)
target_link_libraries(RunImageFilter-Qt
	"-framework OpenGL"
	Qt5::Widgets
	${VUO_FRAMEWORK}
)
