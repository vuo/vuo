cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

set(CMAKE_FRAMEWORK_PATH /Library/Developer/Vuo/framework CACHE STRING "")
find_library(VUO_FRAMEWORK Vuo)

foreach (arch "x86_64" "arm64")
	if (EXISTS "${VUO_FRAMEWORK}/Headers/OBJC-${arch}.h.pch")
		list(APPEND supportedTargets ${arch})
	endif()
endforeach()
set(CMAKE_OSX_ARCHITECTURES ${supportedTargets} CACHE STRING "")

project(CustomType)

set(userModules "~/Library/Application\ Support/Vuo/Modules")
file(MAKE_DIRECTORY ${userModules})

function (VuoCompileModule source bitcodeExtension isCompoundType)
	get_filename_component(sourceName ${source} NAME_WLE)
	set(compiledModule "${sourceName}.${bitcodeExtension}")
	if (${isCompoundType})
		set(depends "")
		set(sourceDir "${VUO_FRAMEWORK}/Modules")
	else()
		set(depends ${source})
		set(sourceDir "${PROJECT_SOURCE_DIR}")
	endif()

	set(bitcodeParts "")
	list(LENGTH CMAKE_OSX_ARCHITECTURES archCount)
	foreach (arch ${CMAKE_OSX_ARCHITECTURES})
		if (archCount EQUAL 1)
			set(bitcode "${sourceName}.${bitcodeExtension}")
		else()
			set(bitcode "${sourceName}-${arch}.${bitcodeExtension}")
		endif()
		add_custom_command(
			DEPENDS ${depends}
			COMMENT "Compiling ${source} to bitcode (${arch})"
			COMMAND "${VUO_FRAMEWORK}/../vuo-compile"
				"${sourceDir}/${source}"
				--header-search-path "${PROJECT_SOURCE_DIR}"
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
			COMMENT "Merging bitcodes into multi-architecture file"
			COMMAND lipo -create ${bitcodeParts} -output ${compiledModule}
			OUTPUT ${compiledModule}
		)
	endif()

	return(PROPAGATE compiledModule)
endfunction()

# Compile the node classes.
foreach (nodeClass "example.player.make.c" "example.player.award.c" "example.player.combine.cc")
	VuoCompileModule(${nodeClass} "vuonode" FALSE)
	list(APPEND compiledNodeClasses ${compiledModule})
endforeach()
add_custom_target(nodeClasses DEPENDS ${compiledNodeClasses})

# Compile the types.
foreach (type "Player.c")
	VuoCompileModule(${type} "bc" FALSE)
	list(APPEND compiledTypes ${compiledModule})
endforeach()
add_custom_target(types DEPENDS ${compiledTypes})
target_sources(types PRIVATE
	Player.h
)

# Compile the list types referenced by the node classes and types above.
foreach (listType "VuoList_Player.cc")
	VuoCompileModule(${listType} "bc" TRUE)
	list(APPEND compiledListTypes ${compiledModule})
endforeach()
add_custom_target(listTypes DEPENDS ${compiledListTypes})

# Package the compiled modules, header files, and example compositions into a node set.
set(nodeSetName "example.player")
set(nodeSetFile "${nodeSetName}.vuonode")
file(GLOB headers ${PROJECT_SOURCE_DIR}/*.h)
file(GLOB examples ${PROJECT_SOURCE_DIR}/examples/*.vuo)
add_custom_command(
	DEPENDS
		${compiledNodeClasses}
		${compiledTypes}
		${compiledListTypes}
		${headers}
		${examples}
	COMMENT "Packaging node set and copying to User Modules folder"
	COMMAND mkdir -p ${nodeSetName}
	COMMAND mkdir -p ${nodeSetName}/examples
	COMMAND cp ${compiledNodeClasses} ${compiledTypes} ${compiledListTypes} ${headers} ${nodeSetName}
	COMMAND cp ${examples} ${nodeSetName}/examples
	COMMAND cd ${nodeSetName} && zip -r --quiet "${PROJECT_BINARY_DIR}/${nodeSetFile}" .
	COMMAND cp ${nodeSetFile} ${userModules}
	OUTPUT ${nodeSetFile}
)
add_custom_target(nodeSet ALL DEPENDS ${nodeSetFile})
