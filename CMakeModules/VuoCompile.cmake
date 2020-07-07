# Outputs the current scope's Vuo node set name (the name of the current source directory).
function (VuoGetNodeSetName nodeSetName)
	if ("${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "/type$")
		set(${nodeSetName} vuo.core PARENT_SCOPE)
	elseif ("${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "/type/list$")
		set(${nodeSetName} vuo.core.list PARENT_SCOPE)
	elseif ("${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "/library$")
		set(${nodeSetName} vuo.core PARENT_SCOPE)
	elseif ("${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "/runtime$")
		set(${nodeSetName} vuo.core.runtime PARENT_SCOPE)
	elseif ("${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "/node/vuo\.[a-z0-9]+$"
		 OR "${CMAKE_CURRENT_SOURCE_DIR}" MATCHES "/test/Test[A-Za-z]+$")
		get_filename_component(name "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
		set(${nodeSetName} ${name} PARENT_SCOPE)
	else ()
		message(FATAL_ERROR "Only use VuoGetNodeSetName() in a node set folder, or the core type, list, library, or runtime folders, or a test folder.")
	endif()
endfunction()


set_property(GLOBAL PROPERTY VuoNodeSets)

# A list of node/type/library object files that are used to generate the built-in Vuo.framework cache.
set_property(GLOBAL PROPERTY VuoCacheableObjects)

set(VuoExampleGlobs
	examples/*.vuo
	examples/*.3ds
	examples/*.csv
	examples/*.dae
	examples/*.data
	examples/*.jpg
	examples/*.mov
	examples/*.mp3
	examples/*.png
)

# Combines bitcode, native objects, source files, examples, and descriptions into a `.vuonode` zip archive.
function (VuoNodeSet)
	VuoGetNodeSetName(nodeSetName)

	# Types.
	if (TARGET "${nodeSetName}.types")
		list(APPEND targets "${nodeSetName}.types")
		get_target_property(typeObjects "${nodeSetName}.types" SOURCES)
		# typeObjects now contains a list of .o files.
		# Bundle bitcode so we can use LLVM's ParseBitcodeFile to load it into our compiler environment,
		# and bundle native objects we can pass them to `ld`
		# without requiring Clang to convert the bitcode files to objects every time.
		set(typeBitcodes ${typeObjects})
		list(TRANSFORM typeBitcodes REPLACE "\\.o$" ".bc")
	endif()

	# Libraries.
	if (TARGET "${nodeSetName}.libraries")
		list(APPEND targets "${nodeSetName}.libraries")
		get_target_property(libraryBitcodes "${nodeSetName}.libraries" SOURCES)
		# libraryBitcodes now contains a list of .o files.
		# Bundle just the bitcode.
		# @todo would it improve performance to bundle native objects?
		list(TRANSFORM libraryBitcodes REPLACE "\\.o$" ".bc")
	endif()

	# Node bitcode.
	if (TARGET "${nodeSetName}.nodes")
		list(APPEND targets "${nodeSetName}.nodes")
		get_target_property(nodeBitcodes "${nodeSetName}.nodes" SOURCES)
	endif()

	# Generic node bitcode and source code.
	if (TARGET "${nodeSetName}.nodes.generic")
		list(APPEND targets "${nodeSetName}.nodes.generic")
		get_target_property(nodeGenericBitcodes "${nodeSetName}.nodes.generic" SOURCES)
	endif()
	if (TARGET "${nodeSetName}.nodes.generic.source")
		list(APPEND targets "${nodeSetName}.nodes.generic.source")
		get_target_property(nodeGenericSources "${nodeSetName}.nodes.generic.source" SOURCES)
	endif()

	# Vuo Pro libraries.
	if (TARGET "${nodeSetName}.libraries.pro")
		list(APPEND targets "${nodeSetName}.libraries.pro")
		get_target_property(libraryProBitcodes "${nodeSetName}.libraries.pro" SOURCES)
		list(FILTER libraryProBitcodes EXCLUDE REGEX "\.o$")
	endif()

	# Vuo Pro node bitcode.
	if (TARGET "${nodeSetName}.nodes.pro")
		list(APPEND targets "${nodeSetName}.nodes.pro")
		get_target_property(nodeProBitcodes "${nodeSetName}.nodes.pro" SOURCES)
	endif()

	# Examples.
	file(GLOB examples RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${VuoExampleGlobs})

	# Descriptions.
	file(GLOB descriptions RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
		descriptions/*.md
		descriptions/*.png
	)

	# Headers.
	# We need to bundle these in order to build generic nodes.
	file(GLOB headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
		*.h
		*.hh
	)

	set(builtFiles
		${typeObjects}
		${typeBitcodes}
		${libraryBitcodes}
		${libraryProBitcodes}
		${nodeBitcodes}
		${nodeGenericBitcodes}
		${nodeProBitcodes}
		${proNodes}
	)
	set(sourceFiles
		${nodeGenericSources}
		${examples}
		${descriptions}
		${headers}
	)
	add_custom_command(
		DEPENDS
			${builtFiles}
			${sourceFiles}
			${targets}
		COMMENT "Archiving ${nodeSetName}"
		# Run `zip` twice, to collect files from different folders while using relative paths in the zip.
		# @todo does 7zip compress faster and/or produce smaller files?
		COMMAND cd ${CMAKE_CURRENT_BINARY_DIR} && zip --quiet --must-match ${CMAKE_CURRENT_BINARY_DIR}/${nodeSetName}.vuonode ${builtFiles}
		COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && zip --quiet --must-match ${CMAKE_CURRENT_BINARY_DIR}/${nodeSetName}.vuonode ${sourceFiles}
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${nodeSetName}.vuonode
	)

	add_custom_target(${nodeSetName} DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${nodeSetName}.vuonode)

	get_property(VuoNodeSets GLOBAL PROPERTY VuoNodeSets)
	list(APPEND VuoNodeSets ${nodeSetName})
	set_property(GLOBAL PROPERTY VuoNodeSets "${VuoNodeSets}")

	set(builtFilesWithPath ${builtFiles})
	list(TRANSFORM builtFilesWithPath PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)
	set_property(GLOBAL APPEND PROPERTY VuoCacheableObjects ${builtFilesWithPath})
endfunction()


# Help Qt Creator's C++ Code Model parse nodes/types/libraries.
# Qt Creator uses Vuo's `*.source` targets to create a context for its C++ Code Model
# (it ignores the properties on the binary targets),
# so we need to copy the relevant properties from the binary target to the source target.
function (VuoCompileSetSourceListProperties target)
	set(qtCreatorIncludeDirectories
		${PROJECT_SOURCE_DIR}/library
		${PROJECT_SOURCE_DIR}/node
		${PROJECT_SOURCE_DIR}/type
		${PROJECT_SOURCE_DIR}/runtime
		${PROJECT_BINARY_DIR}/type/list
		$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>
	)
	set_target_properties(${target}.source PROPERTIES INCLUDE_DIRECTORIES "${qtCreatorIncludeDirectories}")
	set_target_properties(${target}.source PROPERTIES COMPILE_FLAGS "-include VuoHeap.h -include stdbool.h")
endfunction()


# Compiles the specified type source files using `vuo-compile`,
# then converts them to native objects.
function (VuoCompileTypes)
	VuoGetNodeSetName(nodeSetName)
	set(typeSources ${ARGV})
	set(target "${nodeSetName}.types")

	if (VUO_COMPILER_DEVELOPER)
		set(dependsOnVuoCompile "")
	else()
		set(dependsOnVuoCompile "vuo-compile")
	endif()

	if (${nodeSetName} MATCHES ^Test)
		set(exclude EXCLUDE_FROM_ALL)
	endif()

	# Compile sources into bitcode.
	foreach (typeSource ${typeSources})
		get_filename_component(typeBitcode ${typeSource} NAME_WLE)
		set(typeBitcode "${typeBitcode}.bc")
		add_custom_command(
			DEPENDS
				${dependsOnVuoCompile}
				VuoCoreTypesHeader
				${CMAKE_CURRENT_SOURCE_DIR}/${typeSource}
			IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/${typeSource}
			COMMENT "Compiling ${nodeSetName} type ${typeSource}"
			COMMAND_EXPAND_LISTS
			COMMAND ${PROJECT_BINARY_DIR}/bin/vuo-compile
				-I${PROJECT_BINARY_DIR}/type/list
				"-I$<JOIN:$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,;-I>"
				${CMAKE_CURRENT_SOURCE_DIR}/${typeSource}
				-o ${CMAKE_CURRENT_BINARY_DIR}/${typeBitcode}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${typeBitcode}
		)
		list(APPEND typeBitcodes ${typeBitcode})
	endforeach()

	# Convert bitcode into native objects.
	# (Unlike VuoCompileLibraries, we _do_ want to convert the LLVM bitcode
	# to native objects, since we need the generated _retain/_release functions.)
	foreach (typeBitcode ${typeBitcodes})
		get_filename_component(typeObject ${typeBitcode} NAME_WLE)
		set(typeObject "${typeObject}.o")
		add_custom_command(
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${typeBitcode}
			COMMENT "Converting ${nodeSetName} type ${typeBitcode} to .o"
			COMMAND ${CMAKE_CXX_COMPILER}
				-Oz
				-c ${CMAKE_CURRENT_BINARY_DIR}/${typeBitcode}
				-o ${CMAKE_CURRENT_BINARY_DIR}/${typeObject}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${typeObject}
		)
		list(APPEND typeObjects ${typeObject})
	endforeach()

	add_library(${target} STATIC ${exclude} ${typeBitcodes} ${typeObjects})
	set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
	target_include_directories(${target} INTERFACE .)

	# This target enables the source files to show up in Qt Creator's Projects tree.
	add_library(${target}.source MODULE EXCLUDE_FROM_ALL ${typeSources})
	VuoCompileSetSourceListProperties(${target})
endfunction()


# Compiles the specified generated type source files using `vuo-compile`,
# then converts them to native objects.
function (VuoCompileListTypes)
	VuoGetNodeSetName(nodeSetName)
	set(typeSources ${ARGV})
	set(target "${nodeSetName}.types")

	if (VUO_COMPILER_DEVELOPER)
		set(dependsOnVuoCompile "")
	else()
		set(dependsOnVuoCompile "vuo-compile")
	endif()

	# Compile sources into bitcode.
	foreach (typeSource ${typeSources})
		get_filename_component(typeBitcode ${typeSource} NAME_WLE)
		set(typeBitcode "${typeBitcode}.bc")
		add_custom_command(
			DEPENDS
				${dependsOnVuoCompile}
				VuoCoreTypesHeader
				${CMAKE_CURRENT_BINARY_DIR}/${typeSource}
			IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_BINARY_DIR}/${typeSource}
			COMMENT "Compiling ${nodeSetName} type ${typeSource}"
			COMMAND_EXPAND_LISTS
			COMMAND ${PROJECT_BINARY_DIR}/bin/vuo-compile
				-I${PROJECT_BINARY_DIR}/type/list
				"-I$<JOIN:$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,;-I>"
				${CMAKE_CURRENT_BINARY_DIR}/${typeSource}
				-o ${CMAKE_CURRENT_BINARY_DIR}/${typeBitcode}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${typeBitcode}
		)
		list(APPEND typeBitcodes ${typeBitcode})
	endforeach()

	# Convert bitcode into native objects.
	foreach (typeBitcode ${typeBitcodes})
		get_filename_component(typeObject ${typeBitcode} NAME_WLE)
		set(typeObject "${typeObject}.o")
		add_custom_command(
			DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${typeBitcode}
			COMMENT "Converting ${nodeSetName} type ${typeBitcode} to .o"
			COMMAND ${CMAKE_CXX_COMPILER}
				-Oz
				-c ${CMAKE_CURRENT_BINARY_DIR}/${typeBitcode}
				-o ${CMAKE_CURRENT_BINARY_DIR}/${typeObject}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${typeObject}
		)
		list(APPEND typeObjects ${typeObject})
	endforeach()

	add_library(${target} STATIC ${typeBitcodes} ${typeObjects})
	set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
	target_include_directories(${target} INTERFACE .)

	# This target enables the source files to show up in Qt Creator's Projects tree.
	add_library(${target}.source MODULE EXCLUDE_FROM_ALL ${typeSources})
endfunction()


# Compiles the specified library source files to bitcode using `clang`,
# then converts them to native objects.
function (VuoCompileLibraries)
	VuoGetNodeSetName(nodeSetName)
	VuoCompileLibrariesWithTarget("${nodeSetName}.libraries" ${ARGV})
endfunction()
function (VuoCompileLibrariesWithTarget target)
	set(sources ${ARGN})

	# Turn the compiler flags into a list.
	set(cFlags "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
	separate_arguments(cFlags)
	set(cxxFlags "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
	separate_arguments(cxxFlags)

	# Don't include debug symbols in libraries (pending https://b33p.net/kosada/node/4143).
	# But keep the debug flag (and disable optimization) when building runtime libraries, so `VuoCompilerCodeGenUtilities::generateIsPausedComparison()` works.
	if (NOT "${target}" STREQUAL "vuo.core.runtime.libraries"
		AND NOT "${target}" STREQUAL "vuo.core.runtime.objects")
		list(REMOVE_ITEM cFlags -g)
		list(REMOVE_ITEM cxxFlags -g)
	else()
		list(APPEND cFlags   -O0)
		list(APPEND cxxFlags -O0)
	endif()

	# Compile sources into bitcode.
	set(definitionGen "$<REMOVE_DUPLICATES:$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>>")
	set(includeGen "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
	foreach (source ${sources})
		# Choose the compiler.
		get_filename_component(filetype ${source} LAST_EXT)
		set(compiler ${CMAKE_C_COMPILER_LAUNCHER} ${CMAKE_C_COMPILER})
		set(flags "${cFlags}")
		if (filetype STREQUAL ".cc" OR filetype STREQUAL ".mm")
			set(compiler ${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER})
			set(flags "${cxxFlags}")
		endif()

		get_filename_component(bitcode ${source} NAME_WLE)
		set(bitcode "${bitcode}.bc")

		add_custom_command(
			DEPENDS
				VuoCoreTypesHeader
				${CMAKE_CURRENT_SOURCE_DIR}/${source}
			IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/${source}
			COMMENT "Compiling ${target} (bitcode) ${source}"
			COMMAND_EXPAND_LISTS
			COMMAND ${compiler}
				${flags}
				-DVUO_COMPILER
				"$<$<BOOL:${definitionGen}>:-D$<JOIN:${definitionGen},;-D>>"
				"$<$<BOOL:${includeGen}>:-I$<JOIN:${includeGen},;-I>>"
				-target x86_64-apple-macosx10.10.0
				-fblocks
				-fexceptions
				-emit-llvm
				-isysroot ${CMAKE_OSX_SYSROOT}
				-c ${CMAKE_CURRENT_SOURCE_DIR}/${source}
				-o ${CMAKE_CURRENT_BINARY_DIR}/${bitcode}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${bitcode}
		)
		list(APPEND bitcodes ${bitcode})
	endforeach()

	# Compile sources into native objects.
	# (Don't merely convert the bitcode, since this time we build without the "VUO_COMPILER" define.
	# Fixes duplicate `moduleDetails` symbols when linking, e.g., libVuoGlContext.dylib.)
	foreach (source ${sources})
		# Choose the compiler.
		get_filename_component(filetype ${source} LAST_EXT)
		set(compiler "${CMAKE_C_COMPILER}")
		set(flags "${cFlags}")
		if (filetype STREQUAL ".cc" OR filetype STREQUAL ".mm")
			set(compiler "${CMAKE_CXX_COMPILER}")
			set(flags "${cxxFlags}")
		endif()

		get_filename_component(object ${source} NAME_WLE)
		set(object "${object}.o")

		add_custom_command(
			DEPENDS
				VuoCoreTypesHeader
				${CMAKE_CURRENT_SOURCE_DIR}/${source}
			IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/${source}
			COMMENT "Compiling ${target} (native) ${source}"
			COMMAND_EXPAND_LISTS
			COMMAND ${compiler}
				${flags}
				"$<$<BOOL:${definitionGen}>:-D$<JOIN:${definitionGen},;-D>>"
				"$<$<BOOL:${includeGen}>:-I$<JOIN:${includeGen},;-I>>"
				-target x86_64-apple-macosx10.10.0
				-fblocks
				-fexceptions
				-isysroot ${CMAKE_OSX_SYSROOT}
				-c ${CMAKE_CURRENT_SOURCE_DIR}/${source}
				-o ${CMAKE_CURRENT_BINARY_DIR}/${object}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${object}
		)
		list(APPEND objects ${object})
	endforeach()

	add_library(${target} STATIC ${bitcodes} ${objects})
	set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
	target_include_directories(${target}
		INTERFACE
			.
		PRIVATE
			${PROJECT_BINARY_DIR}/type/list
			${PROJECT_SOURCE_DIR}/library
			${PROJECT_SOURCE_DIR}/node
			${PROJECT_SOURCE_DIR}/runtime
			${PROJECT_SOURCE_DIR}/type
	)

	# This target enables the source files to show up in Qt Creator's Projects tree.
	add_library(${target}.source MODULE EXCLUDE_FROM_ALL ${sources})
	VuoCompileSetSourceListProperties(${target})
endfunction()


# Compiles the specified node source files using `vuo-compile`.
function (VuoCompileNodes)
	VuoGetNodeSetName(nodeSetName)
	set(sources ${ARGV})
	set(target "${nodeSetName}.nodes${VuoNodeSetTargetSuffix}")

	if (VUO_COMPILER_DEVELOPER)
		set(dependsOnVuoCompile "")
		set(dependsOnVuoCompileForFramework "")
	else()
		set(dependsOnVuoCompile "vuo-compile")
		set(dependsOnVuoCompileForFramework "vuo-compile-for-framework" VuoFrameworkModuleCompilers)
	endif()

	if (${nodeSetName} MATCHES ^Test)
		set(exclude EXCLUDE_FROM_ALL)
	endif()

	# Compile sources into bitcode.
	foreach (source ${sources})
		get_filename_component(bitcode ${source} NAME_WLE)
		set(bitcode "${bitcode}.vuonode")

		# Use the framework compiler for ISF files.
		get_filename_component(extension ${source} LAST_EXT)
		if (extension STREQUAL ".fs")
			set(thisDep ${dependsOnVuoCompileForFramework})
			set(thisCompiler ${PROJECT_BINARY_DIR}/bin/vuo-compile-for-framework)
		else()
			set(thisDep ${dependsOnVuoCompile})
			set(thisCompiler ${PROJECT_BINARY_DIR}/bin/vuo-compile)
		endif()

		add_custom_command(
			DEPENDS
				${thisDep}
				VuoCoreTypesHeader
				${CMAKE_CURRENT_SOURCE_DIR}/${source}
			IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/${source}
			COMMENT "Compiling ${nodeSetName} node ${source}"
			COMMAND_EXPAND_LISTS
			COMMAND ${thisCompiler}
				-I${CMAKE_CURRENT_SOURCE_DIR}
				"-I$<JOIN:$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,;-I>"
				${CMAKE_CURRENT_SOURCE_DIR}/${source}
				-o ${CMAKE_CURRENT_BINARY_DIR}/${bitcode}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${bitcode}
		)
		list(APPEND bitcodes ${bitcode})
	endforeach()

	add_library(${target} OBJECT ${exclude} ${bitcodes})
	set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
	target_include_directories(${target}
		PRIVATE
			${PROJECT_BINARY_DIR}/type/list
	)

	# This target enables the source files to show up in Qt Creator's Projects tree.
	add_custom_target(${target}.source SOURCES ${sources})
	VuoCompileSetSourceListProperties(${target})
endfunction()


# Compiles the specified node source files using `vuo-compile`,
# and flags them so their source code is also bundled in the node set.
function (VuoCompileGenericNodes)
	VuoGetNodeSetName(nodeSetName)
	set(sources ${ARGV})

	set(VuoNodeSetTargetSuffix ".generic" CACHE INTERNAL "" FORCE)
	VuoCompileNodes(${sources})
	set(VuoNodeSetTargetSuffix "" CACHE INTERNAL "" FORCE)
endfunction()


# Compiles and links the specified compositions using `vuo-compile`.
function (VuoCompileCompositions target)
	set(sources ${ARGN})

	if (VUO_COMPILER_DEVELOPER)
		set(dependsOnVuoCompile "")
	else()
		set(dependsOnVuoCompile "vuo-compile")
	endif()

	# Compile sources into bitcode.
	foreach (source ${sources})
		get_filename_component(compositionBitcode ${source} NAME_WLE)
		set(compositionBitcode "${compositionBitcode}.bc")
		add_custom_command(
			DEPENDS
				${dependsOnVuoCompile}
				${CMAKE_CURRENT_SOURCE_DIR}/${source}
			COMMENT "Compiling composition ${source}"
			COMMAND ${PROJECT_BINARY_DIR}/bin/vuo-compile-for-framework
				${CMAKE_CURRENT_SOURCE_DIR}/${source}
				-o ${CMAKE_CURRENT_BINARY_DIR}/${compositionBitcode}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${compositionBitcode}
		)
		list(APPEND compositionBitcodes ${compositionBitcode})
	endforeach()

	# Link bitcodes into executables.
	foreach (compositionBitcode ${compositionBitcodes})
		get_filename_component(compositionExecutable ${compositionBitcode} NAME_WLE)
		add_custom_command(
			DEPENDS vuo-link ${CMAKE_CURRENT_BINARY_DIR}/${compositionBitcode}
			COMMENT "Linking composition ${compositionExecutable}"
			COMMAND ${PROJECT_BINARY_DIR}/bin/vuo-link
				${CMAKE_CURRENT_BINARY_DIR}/${compositionBitcode}
				--output ${CMAKE_CURRENT_BINARY_DIR}/${compositionExecutable}
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${compositionExecutable}
		)
		list(APPEND compositionExecutables ${CMAKE_CURRENT_BINARY_DIR}/${compositionExecutable})
	endforeach()

	add_custom_target(${target} DEPENDS ${compositionExecutables})
endfunction()


# Renders a PNG and PDF of the specified nodes or compositions using `vuo-export source`.
function (VuoRender target)
	set(nodes ${ARGN})

	foreach (node ${nodes})
		# Specialize generic nodes, so we get their default constant values
		if (node STREQUAL "vuo.math.count")
			set(node vuo.math.count.VuoReal)
		elseif (node STREQUAL "vuo.math.countWithinRange")
			set(node vuo.math.countWithinRange.VuoReal)
		endif()

		get_filename_component(nodeName ${node} NAME)
		string(REGEX REPLACE "\.vuo$" "" nodeName ${nodeName})
		set(nodePNG "${CMAKE_CURRENT_BINARY_DIR}/image-generated/${nodeName}.png")
		set(nodePDF "${CMAKE_CURRENT_BINARY_DIR}/image-generated/${nodeName}.pdf")
		add_custom_command(
			DEPENDS vuo-export
			COMMENT "Rendering ${nodeName}"
			COMMAND ${PROJECT_BINARY_DIR}/bin/vuo-export source --format=pdf --output ${nodePDF} ${node}
			COMMAND ${PROJECT_BINARY_DIR}/bin/vuo-export source --format=png --output ${nodePNG} ${node}
			COMMAND pngquant ${nodePNG}
			COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/image-generated/${nodeName}-fs8.png ${nodePNG}
			OUTPUT ${nodePDF} ${nodePNG}
		)
		list(APPEND output ${nodePDF} ${nodePNG})
	endforeach()

	add_custom_target(${target} DEPENDS ${output})
endfunction()
