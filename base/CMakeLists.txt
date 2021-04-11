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
	VuoBaseDetail.cc
	VuoCable.cc
	VuoComment.cc
	VuoComposition.cc
	VuoCompositionMetadata.cc
	VuoFileUtilities.cc
	VuoFileUtilitiesCocoa.mm
	VuoFileWatcher.cc
	VuoGenericType.cc
	VuoJsonUtilities.cc
	VuoModule.cc
	VuoNode.cc
	VuoNodeClass.cc
	VuoNodeSet.cc
	VuoPort.cc
	VuoPortClass.cc
	VuoProtocol.cc
	VuoPublishedPort.cc
	VuoRunner.cc
	VuoRunningCompositionLibraries.cc
	VuoShaderFile.cc
	VuoShaderIssues.cc
	VuoStringUtilities.cc
	VuoTimeUtilities.cc
	VuoType.cc
	miniz.c
	shared_mutex.cpp

	# These are part of vuo.core.runtime.libraries, which is linked into the VuoHeap target.
#	VuoException.cc
#	VuoTelemetry.c
#	../runtime/VuoEventLoop.m
#	../runtime/VuoLog.cc
)
add_dependencies(VuoBase VuoCoreTypesHeader)
target_compile_definitions(VuoBase
	PUBLIC
	ZMQ
)
target_include_directories(VuoBase
	PUBLIC
	.

	PRIVATE
		../library
		../runtime
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
