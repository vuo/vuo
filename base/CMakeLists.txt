configure_file(VuoConfig.h.in config/VuoConfig.h)
configure_file(VuoConfig-for-framework.h.in VuoConfig-for-framework.h)


# Generate `coreTypes.h`.
get_directory_property(coreTypeHeaders DIRECTORY ../type DEFINITION VuoCoreTypeHeaders)
add_custom_command(
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/type
	COMMAND ./generateCoreTypesHeader.sh ${coreTypeHeaders} > ${CMAKE_CURRENT_BINARY_DIR}/config/coreTypes.h
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config/coreTypes.h
)
add_custom_target(VuoCoreTypesHeader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/config/coreTypes.h)


add_library(VuoBase STATIC
	VuoBase.cc
	VuoBase.hh
	VuoBaseDetail.cc
	VuoBaseDetail.hh
	VuoCable.cc
	VuoCable.hh
	VuoComment.cc
	VuoComment.hh
	VuoComposition.cc
	VuoComposition.hh
	VuoCompositionMetadata.cc
	VuoCompositionMetadata.hh
	VuoFileUtilities.cc
	VuoFileUtilities.hh
	VuoFileUtilitiesCocoa.hh
	VuoFileUtilitiesCocoa.mm
	VuoFileWatcher.cc
	VuoFileWatcher.hh
	VuoGenericType.cc
	VuoGenericType.hh
	VuoJsonUtilities.cc
	VuoJsonUtilities.hh
	VuoMacOSSDKWorkaround.h
	VuoModule.cc
	VuoModule.hh
	VuoNode.cc
	VuoNode.hh
	VuoNodeClass.cc
	VuoNodeClass.hh
	VuoNodeSet.cc
	VuoNodeSet.hh
	VuoPort.cc
	VuoPort.hh
	VuoPortClass.cc
	VuoPortClass.hh
	VuoProtocol.cc
	VuoProtocol.hh
	VuoPublishedPort.cc
	VuoPublishedPort.hh
	VuoRunner.cc
	VuoRunner.hh
	VuoRunningCompositionLibraries.cc
	VuoRunningCompositionLibraries.hh
	VuoShaderFile.cc
	VuoShaderFile.hh
	VuoShaderIssues.cc
	VuoShaderIssues.hh
	VuoStringUtilities.cc
	VuoStringUtilities.hh
	VuoTimeUtilities.cc
	VuoTimeUtilities.hh
	VuoType.cc
	VuoType.hh
	miniz.c
	miniz.h
	shared_mutex.cpp
	shared_mutex.hh

	# These are part of vuo.core.runtime.libraries, which is linked into the VuoHeap target.
#	VuoException.cc
	VuoException.hh
#	VuoTelemetry.cc
	VuoTelemetry.hh
#	../runtime/VuoEventLoop.m
#	../runtime/VuoLog.cc
)
add_dependencies(VuoBase VuoCoreTypesHeader)
target_compile_definitions(VuoBase
	PUBLIC
	ZMQ
)
if (VUO_RUNNER_TRACE)
	target_compile_definitions(VuoBase PRIVATE VUO_RUNNER_TRACE)
endif()
target_include_directories(VuoBase
	PUBLIC
	.

	PRIVATE
		${PROJECT_BINARY_DIR}/type/list
		../library
		../runtime
		../type
)
target_link_libraries(VuoBase
	PUBLIC
	"-framework Cocoa"
	CONAN_PKG::discount
	CONAN_PKG::jsonc
	CONAN_PKG::zeromq

	PRIVATE
	VuoHeap
)
if (VuoPro)
	include(pro/pro.cmake)
endif()
VuoPCH(VuoBase)

add_library(VuoCompositionStub SHARED
	VuoCompositionStub.c)
target_include_directories(VuoCompositionStub
	PRIVATE
		.
		../library
)
VuoPCH(VuoCompositionStub)
