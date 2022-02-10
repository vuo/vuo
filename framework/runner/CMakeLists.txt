# add_library requires a source file, so give it one.
# https://cmake.org/pipermail/cmake/2010-July/038461.html
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/empty.c)
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.c "")
endif()


add_library(VuoRunner SHARED
	empty.c
)
set(frameworkPath "${PROJECT_BINARY_DIR}/lib/VuoRunner.framework")
set(frameworkVersionPath "${frameworkPath}/Versions/${VuoFrameworkVersion}")
set(frameworkBinaryPath "${frameworkVersionPath}/VuoRunner")
list(APPEND generatedFiles
	${vuoRunnerBinary}
)

# This value is substituted into Info.plist.
set(SHORT_VERSION "${VUO_VERSION_AND_BUILD}")

set_target_properties(VuoRunner PROPERTIES
	FRAMEWORK TRUE
	FRAMEWORK_VERSION 2
	VERSION ${Vuo_VERSION}
	SOVERSION ${VuoFrameworkVersion}.0.0
	MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH @loader_path/Modules
)

get_property(VuoNodeSets GLOBAL PROPERTY VuoNodeSets)
list(APPEND typeTargets
	vuo.core.types
	vuo.core.list.types
)
foreach (nodeSet ${VuoNodeSets})
	if (TARGET "${nodeSet}.types")
		list(APPEND typeTargets "${nodeSet}.types")
	endif()
endforeach()
target_link_libraries(VuoRunner
	PRIVATE
	CONAN_PKG::libxml2
	CONAN_PKG::muparser
	VuoBase
	VuoApp
	VuoGlContext
	VuoGlPool
	VuoHeap
	${typeTargets}
	# Do not link to vuo.core.libraries here.
	# The core libraries include Objective-C classes,
	# and since Objective-C doesn't support namespaces,
	# they clash with those linked to compositions
	# that run in the same process as Vuo.framework.
)

VuoGenerateUnexportedSymbolsList(unexportedSymbolsList "${typeTargets}")
target_link_options(VuoRunner
	PRIVATE
		# Ensure the content of these libraries is included in VuoRunner.framework
		# (since the framework itself doesn't have any code).
		LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libVuoBase.a
		LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libvuo.core.types.a
		LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libvuo.core.list.types.a
		LINKER:-reexport-lc++

		# Reduce file size.
		LINKER:-unexported_symbols_list ${unexportedSymbolsList}
)

file(MAKE_DIRECTORY "${frameworkVersionPath}/Documentation/Licenses")
file(MAKE_DIRECTORY "${frameworkVersionPath}/Modules")
file(CREATE_LINK "Versions/Current/Documentation" "${frameworkPath}/Documentation" SYMBOLIC)
file(CREATE_LINK "Versions/Current/Modules"       "${frameworkPath}/Modules"       SYMBOLIC)

set_target_properties(VuoRunner PROPERTIES VuoGeneratedFiles "${generatedFiles}")

add_custom_command(TARGET VuoRunner POST_BUILD
	COMMAND ${codesignAdHoc} ${frameworkBinaryPath} ${codesignAdHocIgnoreReplace}
)
