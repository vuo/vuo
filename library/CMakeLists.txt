VuoCompileLibraries(
	VuoBase64.cc
	VuoCglPixelFormat.c
	VuoDisplayRefresh.c
	VuoFreeImage.c
	VuoGraphicsLayer.m
	VuoGraphicsView.m
	VuoGraphicsWindow.m
	VuoGraphicsWindowDelegate.m
	VuoGraphicsWindowDrag.mm
	VuoImageAverage.c
	VuoImageBlend.c
	VuoImageBlur.c
	VuoImageConvolve.c
	VuoImageCoreGraphics.c
	VuoImageDilate.c
	VuoImageGet.cc
	VuoImageMapColors.c
	VuoImageRenderer.cc
	VuoImageResize.c
	VuoImageRotate.c
	VuoImageText.cc
	VuoIoReturn.c
	VuoMathExpressionParser.cc
	VuoMeshParametric.cc
	VuoMeshUtility.cc
	VuoMouseUtility.m
	VuoOsStatus.c
	VuoPnpId.c
	VuoPointsParametric.cc
	VuoSceneObjectGet.c
	VuoSceneObjectRenderer.cc
	VuoSceneRenderer.cc
	VuoSceneText.c
	VuoScreenCapture.m
	VuoScreenCommon.mm
	VuoSerializable.cc
	VuoSort.c
	VuoTextHtml.c
	VuoUrlFetch.c
	VuoUrlParser.c
	VuoWindow.m
	VuoWindowRecorder.m
	VuoWindowTextInternal.m
	csgjs.cc
)
target_sources(vuo.core.libraries PRIVATE
	VuoBase64.h
	VuoCglPixelFormat.h
	VuoDisplayRefresh.h
	VuoFreeImage.h
	VuoGraphicsLayer.h
	VuoGraphicsView.h
	VuoGraphicsWindow.h
	VuoGraphicsWindowDelegate.h
	VuoGraphicsWindowDrag.h
	VuoImageAverage.h
	VuoImageBlend.h
	VuoImageBlur.h
	VuoImageConvolve.h
	VuoImageDilate.h
	VuoImageGet.h
	VuoImageRenderer.h
	VuoImageResize.h
	VuoImageRotate.h
	VuoImageText.h
	VuoIoReturn.h
	VuoMathExpressionParser.h
	VuoMeshParametric.h
	VuoMeshUtility.h
	VuoMouseUtility.h
	VuoOsStatus.h
	VuoPnpId.h
	VuoPointsParametric.h
	VuoPool.hh
	VuoSceneObjectGet.h
	VuoSceneObjectRenderer.h
	VuoSceneRenderer.h
	VuoSceneText.h
	VuoScreenCapture.h
	VuoScreenCommon.h
	VuoSerializable.hh
	VuoSmooth.h
	VuoSort.h
	VuoTextHtml.h
	VuoTriggerSet.hh
	VuoUrlFetch.h
	VuoUrlParser.h
	VuoWindow.h
	VuoWindowRecorder.h
	VuoWindowTextInternal.h
)
target_include_directories(vuo.core.libraries SYSTEM
	PRIVATE
	${CONAN_INCLUDE_DIRS_CURL}
	${CONAN_INCLUDE_DIRS_FREEIMAGE}
	${CONAN_INCLUDE_DIRS_LIBXML2}
	${CONAN_INCLUDE_DIRS_MUPARSER}
	${CONAN_INCLUDE_DIRS_OAI}
)
target_include_directories(vuo.core.libraries
	PRIVATE
	../node/vuo.image
	../node/vuo.mouse
	../node/vuo.noise
	../node/vuo.scene
	../node/vuo.ui
)
target_link_libraries(vuo.core.libraries
	PUBLIC
		"-framework CoreGraphics"
		"-framework ImageIO"
		"-framework IOKit"
		"-framework OpenGL"

	# Don't enforce these dependencies, since they're unnecessary for VuoRunner.framework
	# and runtime dylibs (VuoApp, etc).
	# CONAN_PKG::curl
	# CONAN_PKG::freeimage
	# CONAN_PKG::libxml2
	# CONAN_PKG::muparser
	# CONAN_PKG::oai

	PRIVATE
	vuo.noise.libraries
)


VuoCompileRuntimeLibrariesWithTarget(vuo.core.module.libraries
	libmodule.c
)


add_library(VuoHeap SHARED
	VuoHeap.cc
	VuoHeap.h
)
set_target_properties(VuoHeap PROPERTIES
	BUILD_WITH_INSTALL_NAME_DIR TRUE

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
)
if (VUO_HEAP_TRACE)
	target_compile_definitions(VuoHeap PRIVATE VUOHEAP_TRACE)
endif()
if (VUO_HEAP_TRACE_ALL)
	target_compile_definitions(VuoHeap PRIVATE VUOHEAP_TRACEALL)
endif()
if (VUO_COMPILER_DEVELOPER AND CMAKE_GENERATOR STREQUAL "Unix Makefiles")
	# Recompile VuoHeap.cc when any of `VUO_HEAP_TRACE*` change.
	# Normally this happens automatically, but VUO_COMPILER_DEVELOPER sets CMAKE_SKIP_RULE_DEPENDENCY.
	set_source_files_properties(VuoHeap.cc PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/VuoHeap.dir/flags.make")
endif()
target_include_directories(VuoHeap
	PUBLIC
		../runtime

	PRIVATE
		.
)
target_link_libraries(VuoHeap
	PUBLIC
	"-framework CoreFoundation"

	PRIVATE
	CONAN_PKG::jsonc
	CONAN_PKG::zeromq
	vuo.core.runtime.libraries
)
target_link_options(VuoHeap
	PRIVATE
	LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libvuo.core.runtime.libraries.a
)
add_custom_command(TARGET VuoHeap POST_BUILD COMMAND ${codesignAdHoc} ${PROJECT_BINARY_DIR}/lib/libVuoHeap.dylib)
VuoPCH(VuoHeap)


# Generate `vuo-wordmark-teal-cropped.h`.
add_custom_command(
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/documentation/image
	COMMAND xxd -i vuo-wordmark-teal-cropped.pdf ${CMAKE_CURRENT_BINARY_DIR}/vuo-wordmark-teal-cropped.h
	OUTPUT vuo-wordmark-teal-cropped.h
)


# This is a library so that we don't need to modify everything that depends on VuoApp
# (running compositions, input editors, tests) every time we add a dependency.
# This is a dylib so that each process (with one or more running compositions)
# only has once instance of VuoCompositionState::compositionStateKey.
add_library(VuoApp SHARED
	VuoApp.h
	VuoApp.m
	VuoAppAboutBox.h
	VuoAppAboutBox.m
	VuoAppDelegate.h
	VuoAppDelegate.m
	VuoAppMenu.m
	VuoAppSplashView.h
	VuoAppSplashView.m
	VuoAppSplashWindow.h
	VuoAppSplashWindow.m
	vuo-wordmark-teal-cropped.h
)
set_target_properties(VuoApp PROPERTIES
	BUILD_WITH_INSTALL_NAME_DIR TRUE

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE

	# For VUO_PROFILE.
	LINKER_LANGUAGE CXX
)
target_include_directories(VuoApp
	PRIVATE
		${PROJECT_BINARY_DIR}/library  # For vuo-wordmark-teal-cropped.h
		../node
		../type
		../common
)

list(LENGTH CMAKE_OSX_ARCHITECTURES archCount)
if (archCount GREATER 1)
	# Why doesn't CMake automatically add the arch flags here?
	target_compile_options(VuoApp PRIVATE "SHELL:-arch x86_64" "SHELL:-arch arm64")
endif()

target_link_libraries(VuoApp
	PUBLIC
	"-framework AppKit"

	PRIVATE
	VuoHeap
	vuo.core.libraries
	vuo.core.module.libraries
)
add_custom_command(TARGET VuoApp POST_BUILD COMMAND ${codesignAdHoc} ${PROJECT_BINARY_DIR}/lib/libVuoApp.dylib)
VuoPCH(VuoApp ModuleOnly)


add_library(VuoGlContext SHARED
	VuoGlContext.cc
	VuoGlContext.h
)
set_target_properties(VuoGlContext PROPERTIES
	BUILD_WITH_INSTALL_NAME_DIR TRUE

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
)
target_include_directories(VuoGlContext
	PRIVATE
		../library
		../node
		../type
		${PROJECT_BINARY_DIR}/type
)
target_link_libraries(VuoGlContext
	PUBLIC
	"-framework IOKit"
	"-framework OpenGL"
	CONAN_PKG::jsonc

	PRIVATE
	VuoApp
	VuoHeap

	# Don't link to vuo.core.types, since it pulls in lots of dependencies…
#	vuo.core.types
	# …whereas we just need a few types.
	${PROJECT_BINARY_DIR}/library/VuoCglPixelFormat.o
	${PROJECT_BINARY_DIR}/library/VuoOsStatus.o
	${PROJECT_BINARY_DIR}/library/VuoPnpId.o
	${PROJECT_BINARY_DIR}/library/VuoScreenCommon.o
	${PROJECT_BINARY_DIR}/type/VuoInteger.o
	${PROJECT_BINARY_DIR}/type/VuoPoint2d.o
	${PROJECT_BINARY_DIR}/type/VuoReal.o
	${PROJECT_BINARY_DIR}/type/VuoScreen.o
	${PROJECT_BINARY_DIR}/type/VuoText.o
	${PROJECT_BINARY_DIR}/type/VuoList_VuoInteger.o
	${PROJECT_BINARY_DIR}/type/VuoList_VuoPoint2d.o
	${PROJECT_BINARY_DIR}/type/VuoList_VuoReal.o
	${PROJECT_BINARY_DIR}/type/VuoList_VuoScreen.o
	${PROJECT_BINARY_DIR}/type/VuoList_VuoText.o
)
add_dependencies(VuoGlContext
	vuo.core.types
	vuo.core.types.compound
)
VuoPCH(VuoGlContext Module)


add_library(VuoGlPool SHARED
	VuoGlPool.cc
	VuoGlPool.h
)
set_target_properties(VuoGlPool PROPERTIES
	BUILD_WITH_INSTALL_NAME_DIR TRUE

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
)
target_include_directories(VuoGlPool
	PRIVATE
		../library
		../node
		../runtime
		../type
		${PROJECT_BINARY_DIR}/type
)
target_link_libraries(VuoGlPool
	PUBLIC
	"-framework IOSurface"

	PRIVATE
		VuoApp
		VuoBase
		VuoGlContext
)
VuoPCH(VuoGlPool Module)

if (VuoPro)
	include(pro/pro.cmake)
endif()
