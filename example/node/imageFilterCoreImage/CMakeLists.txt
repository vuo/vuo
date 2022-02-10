cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
cmake_policy(VERSION 3.16...3.20)

execute_process(
	COMMAND xcrun --sdk macosx --show-sdk-path
	OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
	COMMAND /usr/bin/plutil -extract SupportedTargets.macosx.Archs json -o - "${CMAKE_OSX_SYSROOT}/SDKSettings.plist"
	OUTPUT_VARIABLE supportedTargets
)
if (supportedTargets MATCHES arm64)
	set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "")
else()
	execute_process(COMMAND /usr/bin/uname -m OUTPUT_VARIABLE systemArch)
	string(STRIP ${systemArch} systemArch)
	set(CMAKE_OSX_ARCHITECTURES ${systemArch} CACHE STRING "" FORCE)
endif()
project(ExampleImageFilterCoreImage)

set(CMAKE_FRAMEWORK_PATH /Library/Developer/Vuo/framework CACHE STRING "")
find_library(VUO_FRAMEWORK Vuo)

set(userModules "~/Library/Application\ Support/Vuo/Modules")
file(MAKE_DIRECTORY ${userModules})

set(nodeSource "${PROJECT_SOURCE_DIR}/example.imageFilter.coreImage.c")
set(librarySource "${PROJECT_SOURCE_DIR}/ExampleImageFilterCoreImage.m")

set(compiledNode "")
set(compiledObject "")
set(staticLibrary "")
set(bitcodeParts "")
set(objectParts "")
set(staticLibraryParts "")
list(LENGTH CMAKE_OSX_ARCHITECTURES archCount)
foreach (arch ${CMAKE_OSX_ARCHITECTURES})
	get_filename_component(bitcode ${nodeSource} NAME_WLE)
	set(compiledNode "${bitcode}.vuonode")
	if (archCount EQUAL 1)
		set(bitcode "${bitcode}.vuonode")
	else()
		set(bitcode "${bitcode}-${arch}.vuonode")
	endif()

	add_custom_command(
		DEPENDS ${nodeSource}
		COMMENT "Compiling node class (${arch})"
		COMMAND ${VUO_FRAMEWORK}/../vuo-compile
			${nodeSource}
			--target ${arch}-apple-macosx10.10.0
			--output ${bitcode}
		OUTPUT ${bitcode}
	)

	get_filename_component(object ${nodeSource} NAME_WLE)
	set(compiledObject "${object}.o")
	if (archCount EQUAL 1)
		set(object "${object}.o")
	else()
		set(object "${object}-${arch}.o")
	endif()
	add_custom_command(
		DEPENDS ${librarySource}
		COMMENT "Compiling library to object (${arch})"
		COMMAND xcrun clang
			-isysroot ${CMAKE_OSX_SYSROOT}
			-target ${arch}-apple-macosx10.10.0
			-I ${VUO_FRAMEWORK}/Headers
			-c ${librarySource}
			-o ${object}
		OUTPUT ${object}
	)

	set(staticLibrary "libExampleImageFilterCoreImage.a")
	if (archCount EQUAL 1)
		set(archive "libExampleImageFilterCoreImage.a")
	else()
		set(archive "libExampleImageFilterCoreImage-${arch}.a")
	endif()
	add_custom_command(
		DEPENDS ${object}
		COMMENT "Creating static library from object (${arch})"
		COMMAND xcrun ar
			-r ${archive}
			${object}
		OUTPUT ${archive}
	)

	if (archCount GREATER 1)
		list(APPEND bitcodeParts ${bitcode})
		list(APPEND objectParts ${object})
		list(APPEND staticLibraryParts ${archive})
	endif()
endforeach()

if (archCount GREATER 1)
	add_custom_command(
		DEPENDS ${bitcodeParts}
		COMMENT "Merging node class parts into multi-architecture file"
		COMMAND lipo -create ${bitcodeParts} -output ${compiledNode}
		OUTPUT ${compiledNode}
	)

	add_custom_command(
		DEPENDS ${staticLibraryParts}
		COMMENT "Merging static library parts into multi-architecture file"
		COMMAND lipo -create ${staticLibraryParts} -output ${staticLibrary}
		OUTPUT ${staticLibrary}
	)
endif()

set(installedNode ${userModules}/${compiledNode})
add_custom_command(
	DEPENDS ${compiledNode} ${staticLibrary}
	COMMENT "Copying to User Modules folder"
	COMMAND cp ${compiledNode} ${staticLibrary} ${userModules}
	OUTPUT ${installedNode}
)

add_custom_target(node ALL
	DEPENDS ${installedNode}
)
target_sources(node PRIVATE ExampleImageFilterCoreImage.h)
