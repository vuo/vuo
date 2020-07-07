cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(ExampleImageFilterCoreImage)

set(CMAKE_FRAMEWORK_PATH ../../../framework)
find_library(VUO_FRAMEWORK Vuo)

set(userModules "~/Library/Application\ Support/Vuo/Modules")
file(MAKE_DIRECTORY ${userModules})

set(compiledNode ${PROJECT_BINARY_DIR}/example.imageFilter.coreImage.vuonode)
add_custom_command(
	COMMAND xcrun clang -I ${VUO_FRAMEWORK}/Headers -c ${PROJECT_SOURCE_DIR}/ExampleImageFilterCoreImage.m -o ${PROJECT_BINARY_DIR}/ExampleImageFilterCoreImage.o
	COMMAND xcrun ar -r ${PROJECT_BINARY_DIR}/libExampleImageFilterCoreImage.a ${PROJECT_BINARY_DIR}/ExampleImageFilterCoreImage.o
	COMMAND ${VUO_FRAMEWORK}/../vuo-compile ${PROJECT_SOURCE_DIR}/example.imageFilter.coreImage.c --output ${compiledNode}
	COMMAND cp ${compiledNode} ${PROJECT_BINARY_DIR}/*.a ${userModules}
	OUTPUT ${compiledNode}
)
add_custom_target(node ALL DEPENDS ${compiledNode})
