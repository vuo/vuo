cmake_minimum_required(VERSION 3.14...3.17 FATAL_ERROR)
project(CustomType)

set(CMAKE_FRAMEWORK_PATH ../../../framework)
find_library(VUO_FRAMEWORK Vuo)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

set(userModules "~/Library/Application\ Support/Vuo/Modules")
file(MAKE_DIRECTORY ${userModules})

# Compile the node class.
set(compiledNode ${PROJECT_BINARY_DIR}/example.customType.greet.vuonode)
add_custom_command(
	COMMAND ${VUO_FRAMEWORK}/../vuo-compile ${PROJECT_SOURCE_DIR}/example.customType.greet.c --output ${compiledNode}
	OUTPUT ${compiledNode}
)
add_custom_target(node DEPENDS ${compiledNode})

# Compile the port type.
set(compiledType
	${PROJECT_BINARY_DIR}/ExampleLanguage.bc
	${PROJECT_BINARY_DIR}/VuoList_ExampleLanguage.bc
)
add_custom_command(
	COMMAND ${VUO_FRAMEWORK}/../vuo-compile ${PROJECT_SOURCE_DIR}/ExampleLanguage.c          --output ${PROJECT_BINARY_DIR}/ExampleLanguage.bc
	COMMAND ${VUO_FRAMEWORK}/../vuo-compile ${PROJECT_SOURCE_DIR}/VuoList_ExampleLanguage.cc --output ${PROJECT_BINARY_DIR}/VuoList_ExampleLanguage.bc
	OUTPUT ${compiledType}
)
add_custom_target(type DEPENDS ${compiledType})

# Package the node class and port type into a node set.
set(nodeset example.customtype.vuonode)
add_custom_command(
	DEPENDS
		node
		type
	COMMAND zip --quiet --junk-paths ${nodeset}
		${compiledNode}
		${compiledType}
		${PROJECT_SOURCE_DIR}/ExampleLanguage.h
		${PROJECT_SOURCE_DIR}/VuoList_ExampleLanguage.h
	COMMAND cp ${nodeset} ${userModules}
	OUTPUT ${nodeset}
)
add_custom_target(nodeset ALL DEPENDS ${nodeset})

# Compile the port type to a .o file, to be linked into the input editor
add_library(typeObject STATIC
	ExampleLanguage.c
)
target_include_directories(typeObject PRIVATE
	${VUO_FRAMEWORK}/Headers
)

# Build the input editor
add_library(ExampleLanguageInputEditor SHARED
	ExampleLanguageInputEditor.cc
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
	typeObject
)
add_custom_command(TARGET ExampleLanguageInputEditor POST_BUILD
	COMMAND cp ${PROJECT_BINARY_DIR}/libExampleLanguageInputEditor.dylib ${userModules}
)
