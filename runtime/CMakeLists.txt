set(prefixHeader " -include ../vuo.pch")
string(APPEND CMAKE_C_FLAGS ${prefixHeader})
string(APPEND CMAKE_CXX_FLAGS ${prefixHeader})

# Objects that should be copied into Vuo.framework/Modules
# (but not linked in to vuo.core.runtime.libraries).
VuoCompileRuntimeLibrariesWithTarget(vuo.core.runtime.objects
	VuoRuntime.cc
	VuoRuntimeMain.c
)
target_sources(vuo.core.runtime.objects PRIVATE
	VuoRuntime.h
)
target_include_directories(vuo.core.runtime.objects PRIVATE
	../base
)
target_include_directories(vuo.core.runtime.objects SYSTEM PRIVATE
	${CONAN_INCLUDE_DIRS_JSONC}
	${CONAN_INCLUDE_DIRS_ZEROMQ}
)

# Objects that should be linked in to vuo.core.runtime.libraries
# (and eventually VuoHeap).
VuoCompileRuntimeLibraries(
	VuoCompositionDiff.cc
	VuoCompositionState.c
	VuoEventLoop.m
	VuoLog.cc
	VuoNodeRegistry.cc
	VuoNodeSynchronization.cc
	VuoRuntimeCommunicator.cc
	VuoRuntimeContext.cc
	VuoRuntimePersistentState.cc
	VuoRuntimeState.cc
	VuoRuntimeUtilities.cc
	VuoThreadManager.cc
	../base/VuoException.cc
	../base/VuoTelemetry.cc
)
target_sources(vuo.core.runtime.libraries PRIVATE
	VuoCompositionDiff.hh
	VuoCompositionState.h
	VuoEventLoop.h
	VuoNodeRegistry.hh
	VuoNodeSynchronization.hh
	VuoRuntimeCommunicator.hh
	VuoRuntimeContext.hh
	VuoRuntimePersistentState.hh
	VuoRuntimeState.hh
	VuoRuntimeUtilities.hh
	VuoThreadManager.hh
	../common/VuoLog.h
	../node/module.h
	../node/node.h
)
target_compile_definitions(vuo.core.runtime.libraries
	PUBLIC
	ZMQ
)
target_include_directories(vuo.core.runtime.libraries PRIVATE
	../base
)
target_include_directories(vuo.core.runtime.libraries SYSTEM PUBLIC
	${CONAN_INCLUDE_DIRS_JSONC}
	${CONAN_INCLUDE_DIRS_ZEROMQ}
)
target_link_libraries(vuo.core.runtime.libraries PUBLIC
	"-framework AppKit"
	"-framework IOKit"
)
string(REPLACE ${prefixHeader} "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
string(REPLACE ${prefixHeader} "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})


# Build VuoCompositionLoader.
add_executable(VuoCompositionLoader MACOSX_BUNDLE
	VuoCompositionLoader.cc
)
# This value is substituted into Info.plist.
set(SHORT_VERSION "${VUO_VERSION_AND_BUILD}")
set_target_properties(VuoCompositionLoader PROPERTIES
	MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/VuoCompositionLoader-Info.plist

	# Don't automatically add `rpath` search paths to this executable.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH @loader_path/../../../../Modules
)
target_include_directories(VuoCompositionLoader
	PRIVATE
		../library
)
VuoBundleFiles(VuoCompositionLoader "Resources"
	../editor/VuoEditorApp/Icons/vuo-composition.icns
)
target_link_options(VuoCompositionLoader
	PRIVATE
	# Enable nodes/libraries to look up these symbols with dlsym(),
	# even after the other symbols get stripped during packaging.
	LINKER:-exported_symbol,_VuoApp_mainThread
	LINKER:-exported_symbol,_VuoApp_dylibPath
)
target_link_libraries(VuoCompositionLoader
	VuoBase
)
VuoPCH(VuoCompositionLoader)
