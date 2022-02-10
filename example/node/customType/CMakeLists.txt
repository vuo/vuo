cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
cmake_policy(VERSION 3.16...3.20)

execute_process(
	COMMAND xcrun --sdk macosx --show-sdk-path
	OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
project(CustomType)

set(CMAKE_FRAMEWORK_PATH /Library/Developer/Vuo/framework CACHE STRING "")
find_library(VUO_FRAMEWORK Vuo)

find_package(Qt5 REQUIRED COMPONENTS Widgets)


# Build for the architectures supported by both the macOS SDK and the found Qt package.
execute_process(
	COMMAND /usr/bin/plutil -extract SupportedTargets.macosx.Archs json -o - "${CMAKE_OSX_SYSROOT}/SDKSettings.plist"
	OUTPUT_VARIABLE MACOS_SDK_ARCHITECTURES
)
if (NOT MACOS_SDK_ARCHITECTURES MATCHES x86_64 AND NOT MACOS_SDK_ARCHITECTURES MATCHES arm64)
	# Xcode 9 and earlier don't have a `SupportedTargets.macosx.Archs`; it's safe to assume they only support x86_64.
	set(MACOS_SDK_ARCHITECTURES x86_64)
endif()
execute_process(
	COMMAND xcrun lipo -info "${Qt5_DIR}/../../QtWidgets.framework/QtWidgets"
	OUTPUT_VARIABLE Qt5_ARCHITECTURES
)
if (MACOS_SDK_ARCHITECTURES MATCHES x86_64 AND Qt5_ARCHITECTURES MATCHES x86_64)
	list(APPEND buildArchitectures x86_64)
endif()
if (MACOS_SDK_ARCHITECTURES MATCHES arm64 AND Qt5_ARCHITECTURES MATCHES arm64)
	list(APPEND buildArchitectures arm64)
endif()
set(CMAKE_OSX_ARCHITECTURES ${buildArchitectures} CACHE STRING "" FORCE)


set(userModules "~/Library/Application\ Support/Vuo/Modules")
file(MAKE_DIRECTORY ${userModules})

macro(VuoCompileModule source bitcodeExtension shouldCompileObject)
	get_filename_component(sourceName ${source} NAME_WLE)
	set(compiledModule "${sourceName}.${bitcodeExtension}")
	set(compiledObject "${sourceName}.o")

	set(bitcodeParts "")
	set(objectParts "")
	list(LENGTH CMAKE_OSX_ARCHITECTURES archCount)
	foreach (arch ${CMAKE_OSX_ARCHITECTURES})
		if (archCount EQUAL 1)
			set(bitcode "${sourceName}.${bitcodeExtension}")
		else()
			set(bitcode "${sourceName}-${arch}.${bitcodeExtension}")
		endif()
		add_custom_command(
			DEPENDS ${source}
			COMMENT "Compiling ${source} to bitcode (${arch})"
			COMMAND ${VUO_FRAMEWORK}/../vuo-compile
				${source}
				--target ${arch}-apple-macosx10.10.0
				--output ${bitcode}
			OUTPUT ${bitcode}
		)

		if (${shouldCompileObject})
			if (archCount EQUAL 1)
				set(object "${sourceName}.o")
			else()
				set(object "${sourceName}-${arch}.o")
			endif()
			add_custom_command(
				DEPENDS ${bitcode}
				COMMENT "Converting bitcode to object"
				COMMAND ${CMAKE_CXX_COMPILER}
					-target ${arch}-apple-macosx10.10.0
					-Oz
					-c ${bitcode}
					-o ${object}
				OUTPUT ${object}
			)
		endif()

		if (archCount GREATER 1)
			list(APPEND bitcodeParts ${bitcode})
			list(APPEND objectParts ${object})
		endif()
	endforeach()

	if (archCount GREATER 1)
		add_custom_command(
			DEPENDS ${bitcodeParts}
			COMMENT "Merging bitcodes into multi-architecture file"
			COMMAND lipo -create ${bitcodeParts} -output ${compiledModule}
			OUTPUT ${compiledModule}
		)

		if (${shouldCompileObject})
			add_custom_command(
				DEPENDS ${objectParts}
				COMMENT "Merging objects into multi-architecture file"
				COMMAND lipo -create ${objectParts} -output ${compiledObject}
				OUTPUT ${compiledObject}
			)
		endif()
	endif()
endmacro()

# Compile the node class.
VuoCompileModule("${PROJECT_SOURCE_DIR}/example.customType.greet.c" "vuonode" FALSE)
set(compiledNode ${compiledModule})
add_custom_target(node DEPENDS ${compiledNode})

# Compile the port type.
VuoCompileModule("${PROJECT_SOURCE_DIR}/ExampleLanguage.c" "bc" TRUE)
set(compiledType ${compiledModule})
set(compiledTypeObject ${compiledObject})
VuoCompileModule("${PROJECT_SOURCE_DIR}/VuoList_ExampleLanguage.cc" "bc" FALSE)
set(compiledListType ${compiledModule})
add_custom_target(type DEPENDS ${compiledType} ${compiledTypeObject} ${compiledListType})
target_sources(type PRIVATE
	ExampleLanguage.h
	VuoList_ExampleLanguage.h
)

# Package the node class and port type into a node set.
set(nodeset example.customtype.vuonode)
add_custom_command(
	DEPENDS
		node
		type
	COMMENT "Packaging node set and copying to User Modules folder"
	COMMAND zip --quiet --junk-paths ${nodeset}
		${compiledNode}
		${compiledType}
		${PROJECT_SOURCE_DIR}/ExampleLanguage.h
		${PROJECT_SOURCE_DIR}/VuoList_ExampleLanguage.h
	COMMAND cp ${nodeset} ${userModules}
	OUTPUT ${nodeset}
)
add_custom_target(nodeset ALL DEPENDS ${nodeset})

# Build the input editor
add_library(ExampleLanguageInputEditor SHARED
	ExampleLanguageInputEditor.cc
	ExampleLanguageInputEditor.hh
	${compiledTypeObject}
)
set_target_properties(ExampleLanguageInputEditor PROPERTIES
	AUTOMOC ON

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
)
target_compile_options(ExampleLanguageInputEditor PRIVATE
	-include${VUO_FRAMEWORK}/../resources/inputEditorWidgets/widget.pch
)
target_include_directories(ExampleLanguageInputEditor PRIVATE
	${VUO_FRAMEWORK}/Headers
	${VUO_FRAMEWORK}/../resources/inputEditorWidgets
)
target_link_directories(ExampleLanguageInputEditor PRIVATE
	${VUO_FRAMEWORK}/Modules
)
target_link_libraries(ExampleLanguageInputEditor PRIVATE
	Qt5::Widgets
	${VUO_FRAMEWORK}
	${VUO_FRAMEWORK}/../resources/inputEditorWidgets/libVuoWidget.dylib
)
add_custom_command(TARGET ExampleLanguageInputEditor POST_BUILD
	COMMENT "Building input editor, ad-hoc code-signing, and copying to User Modules folder"
	COMMAND codesign --sign - --force ${PROJECT_BINARY_DIR}/libExampleLanguageInputEditor.dylib
	COMMAND cp ${PROJECT_BINARY_DIR}/libExampleLanguageInputEditor.dylib ${userModules}
)
