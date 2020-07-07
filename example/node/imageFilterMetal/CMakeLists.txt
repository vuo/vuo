cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(ExampleImageFilterMetal)

set(CMAKE_FRAMEWORK_PATH ../../../framework)
find_library(VUO_FRAMEWORK Vuo)

set(userModules "~/Library/Application\ Support/Vuo/Modules")
file(MAKE_DIRECTORY ${userModules})

set(compiledNode ${PROJECT_BINARY_DIR}/example.imageFilter.metal.vuonode)
add_custom_command(
	COMMAND xcrun clang -mmacosx-version-min=10.11 -I ${VUO_FRAMEWORK}/Headers -c ${PROJECT_SOURCE_DIR}/ExampleImageFilterMetal.m -o ${PROJECT_BINARY_DIR}/ExampleImageFilterMetal.o
	COMMAND xcrun ar -r ${PROJECT_BINARY_DIR}/libExampleImageFilterMetal.a ${PROJECT_BINARY_DIR}/ExampleImageFilterMetal.o
	COMMAND ${VUO_FRAMEWORK}/../vuo-compile ${PROJECT_SOURCE_DIR}/example.imageFilter.metal.c --output ${compiledNode}
	COMMAND cp ${compiledNode} ${PROJECT_BINARY_DIR}/*.a ${userModules}
	OUTPUT ${compiledNode}
)
add_custom_target(node ALL DEPENDS ${compiledNode})
