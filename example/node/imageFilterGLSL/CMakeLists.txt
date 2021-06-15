cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
cmake_policy(VERSION 3.16...3.20)

set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "")
project(ExampleImageFilterGLSL LANGUAGES NONE)

set(CMAKE_FRAMEWORK_PATH /Library/Developer/Vuo/framework CACHE STRING "")
find_library(VUO_FRAMEWORK Vuo)

set(userModules "~/Library/Application\ Support/Vuo/Modules")
file(MAKE_DIRECTORY ${userModules})

set(source "${PROJECT_SOURCE_DIR}/example.imageFilter.brightness.c")

set(compiledNode "")
set(bitcodeParts "")
list(LENGTH CMAKE_OSX_ARCHITECTURES archCount)
foreach (arch ${CMAKE_OSX_ARCHITECTURES})
	get_filename_component(bitcode ${source} NAME_WLE)
	set(compiledNode "${bitcode}.vuonode")
	if (archCount EQUAL 1)
		set(bitcode "${bitcode}.vuonode")
	else()
		set(bitcode "${bitcode}-${arch}.vuonode")
	endif()

	add_custom_command(
		DEPENDS ${source}
		COMMENT "Compiling node class (${arch})"
		COMMAND ${VUO_FRAMEWORK}/../vuo-compile
			${source}
			--target ${arch}-apple-macosx10.10.0
			--output ${bitcode}
		OUTPUT ${bitcode}
	)

	if (archCount GREATER 1)
		list(APPEND bitcodeParts ${bitcode})
	endif()
endforeach()

if (archCount GREATER 1)
	add_custom_command(
		DEPENDS ${bitcodeParts}
		COMMENT "Merging into multi-architecture file"
		COMMAND lipo -create ${bitcodeParts} -output ${compiledNode}
		OUTPUT ${compiledNode}
	)
endif()

set(installedNode ${userModules}/${compiledNode})
add_custom_command(
	DEPENDS ${compiledNode}
	COMMENT "Copying to User Modules folder"
	COMMAND cp ${compiledNode} ${installedNode}
	OUTPUT ${installedNode}
)

add_custom_target(node ALL
	DEPENDS ${installedNode}
)
