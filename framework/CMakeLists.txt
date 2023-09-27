add_subdirectory(llvm)
add_subdirectory(runner)


# add_library requires a source file, so give it one.
# https://cmake.org/pipermail/cmake/2010-July/038461.html
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/empty.c)
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.c "")
endif()


# Provide separate VuoFramework and VuoFrameworkDylib targets,
# so other targets can link to `Vuo.framework/Vuo` (dependency on VuoFrameworkDylib)
# and then be copied into `Vuo.framework` (dependency of VuoFramework).
add_custom_target(VuoFramework)

set(frameworkPath "${PROJECT_BINARY_DIR}/lib/Vuo.framework")
set(frameworkVersionPath "${frameworkPath}/Versions/${VuoFrameworkVersion}")

add_library(VuoFrameworkDylib SHARED
	empty.c
)

# This value is substituted into Info.plist.
set(SHORT_VERSION "${VUO_VERSION_AND_BUILD}")

set_target_properties(VuoFrameworkDylib PROPERTIES
	OUTPUT_NAME "Vuo"
	FRAMEWORK TRUE
	FRAMEWORK_VERSION 2
	VERSION ${Vuo_VERSION}
	SOVERSION ${VuoFrameworkVersion}.0.0
	MACOSX_FRAMEWORK_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist

	# Don't automatically add `rpath` search paths to this dylib.
	BUILD_WITH_INSTALL_RPATH TRUE
	INSTALL_RPATH @loader_path/Modules
)
target_compile_options(VuoFrameworkDylib
	INTERFACE
	-F${PROJECT_BINARY_DIR}/lib
)
target_include_directories(VuoFrameworkDylib
	INTERFACE
		${frameworkPath}/Headers
)
list(APPEND generatedFiles
	${frameworkVersionPath}/Vuo
)
add_dependencies(VuoFramework VuoFrameworkDylib)


get_property(VuoNodeSets GLOBAL PROPERTY VuoNodeSets)
list(APPEND typeTargets
	vuo.core.types
	vuo.core.types.compound
)
foreach (nodeSet ${VuoNodeSets})
	if (TARGET "${nodeSet}.types")
		list(APPEND typeTargets "${nodeSet}.types")
		list(APPEND typeLinkerFlags LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/lib${nodeSet}.types.a)
	endif()
	if (TARGET "${nodeSet}.types.compound")
		list(APPEND typeTargets "${nodeSet}.types.compound")
		list(APPEND typeLinkerFlags LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/lib${nodeSet}.types.compound.a)
	endif()
	if (TARGET "${nodeSet}.libraries")
		list(APPEND libraryTargets "${nodeSet}.libraries")
	endif()
endforeach()
target_link_libraries(VuoFrameworkDylib
	PRIVATE
	CONAN_PKG::curl
	CONAN_PKG::freeimage
	CONAN_PKG::libxml2
	CONAN_PKG::muparser
	CONAN_PKG::oai
	VuoBase
	VuoCompiler
	VuoRunner
	VuoRunnerCocoa
	VuoApp
	VuoGlContext
	VuoGlPool
	VuoHeap
	${typeTargets}
	${libraryTargets}
	# Do not link to vuo.core.libraries here.
	# The core libraries include Objective-C classes,
	# and since Objective-C doesn't support namespaces,
	# they clash with those linked to compositions
	# that run in the same process as Vuo.framework.
)
add_custom_command(TARGET VuoFrameworkDylib POST_BUILD
	COMMAND install_name_tool -change @rpath/libc++.dylib /usr/lib/libc++.1.dylib ${frameworkVersionPath}/Vuo
	COMMAND install_name_tool -change @loader_path/../Frameworks/Syphon.framework/Versions/A/Syphon @rpath/../Frameworks/Syphon.framework/Syphon ${frameworkVersionPath}/Vuo
	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Vuo
)


# Ensure the content of these libraries is included in Vuo.framework
# (since the framework itself doesn't have any code).
target_link_options(VuoFrameworkDylib
	PRIVATE
	LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libVuoBase.a
	LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libVuoCompiler.a
	LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libVuoRunner.a
	LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libVuoRunnerCocoa.a
	LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libvuo.core.types.a
	LINKER:-force_load,${PROJECT_BINARY_DIR}/lib/libvuo.core.types.compound.a
	${typeLinkerFlags}  # Force non-core types' functions to be loaded — needed for VuoCompilerPublishedPort::getDetails() and input editors.
)


# Make JSON-C and Vuo's dylibs available to Vuo.framework users.
target_link_options(VuoFrameworkDylib
	PRIVATE
	LINKER:-reexport_library,${CONAN_JSONC_ROOT}/lib/libjson-c.dylib
	LINKER:-reexport_library,${PROJECT_BINARY_DIR}/lib/libVuoApp.dylib
	LINKER:-reexport_library,${PROJECT_BINARY_DIR}/lib/libVuoGlContext.dylib
	LINKER:-reexport_library,${PROJECT_BINARY_DIR}/lib/libVuoGlPool.dylib
	LINKER:-reexport_library,${PROJECT_BINARY_DIR}/lib/libVuoHeap.dylib
)


# Make LLVM/Clang available to Vuo.framework users.
string(JOIN " LINKER:-reexport-l" llvmReexports ${CONAN_LIBS_LLVM})
set(llvmReexports "LINKER:-reexport-l${llvmReexports}")
separate_arguments(llvmReexports)
target_link_options(VuoFrameworkDylib PRIVATE -L${CONAN_LIB_DIRS_LLVM} ${llvmReexports})


# Reduce file size.
VuoGenerateUnexportedSymbolsList(unexportedSymbolsList "${typeTargets}")
target_link_options(VuoFrameworkDylib
	PRIVATE
		LINKER:-unexported_symbols_list ${unexportedSymbolsList}
)


# Help the system linker on old macOS versions (10.11) find the framework's dependencies.
# (More recent `ld64` versions automatically find them using the dylib's built-in rpath.)
target_link_options(VuoFrameworkDylib
	PUBLIC
	-L${frameworkPath}/Modules
)


VuoBundleFiles(VuoFrameworkDylib "Resources"
	drivers/imageFilterDriver.vuo
	drivers/imageGeneratorDriver.vuo
	drivers/imageTransitionDriver.vuo
	templates/FFGLBlendMode.vuo
	templates/FFGLEffect.vuo
	templates/FFGLSource.vuo
	templates/FxPlugEffect.vuo
	templates/FxPlugGenerator.vuo
	templates/FxPlugTransition.vuo
	templates/GLSLImageFilter.fs
	templates/GLSLImageFilter.vs
	templates/GLSLImageGenerator.fs
	templates/GLSLImageGenerator.vs
	templates/GLSLImageTransition.fs
	templates/GLSLImageTransition.vs
	templates/GLSLObjectFilter.gs
	templates/GLSLObjectFilter.vs
	templates/GLSLObjectRenderer.fs
	templates/GLSLObjectRenderer.gs
	templates/GLSLObjectRenderer.vs
	templates/imageTemplate.vuo
	templates/layersTemplate.vuo
	templates/movie.vuo
	templates/sceneTemplate.vuo
	templates/screensaver.vuo
)

VuoBundleFiles(VuoFrameworkDylib "Resources/shaders"
	shaders/GPUNoiseLib.glsl
	shaders/ISFGLMacro2D.txt
	shaders/ISFGLMacro2DBias.txt
	shaders/ISFGLMacro2DRect.txt
	shaders/ISFGLMacro2DRectBias.txt
	shaders/ISFGLScenePassthru.vs
	shaders/ISFGLSceneVertShaderIncludeInitFunc.txt
	shaders/ISFGLSceneVertShaderIncludeVarDec.txt
	shaders/README.md
	shaders/VuoGlslAlpha.glsl
	shaders/VuoGlslBrightness.glsl
	shaders/VuoGlslHsl.glsl
	shaders/VuoGlslProjection.glsl
	shaders/VuoGlslRandom.glsl
	shaders/VuoGlslTangent.glsl
	shaders/VuoSurfaceUnlit.glsl
	shaders/ashima-math.glsl
	shaders/ashima-matrix.glsl
	shaders/deform.glsl
	shaders/lighting.glsl
	shaders/noise2D.glsl
	shaders/noise3D.glsl
	shaders/noise4D.glsl
	shaders/shadertoy-prefix.glsl
	shaders/shadertoy-suffix.glsl
	shaders/shadertoy.vs
	shaders/triangle.glsl
	shaders/triangleLine.glsl
	shaders/trianglePoint.glsl
)


set(installNameToolIgnoreSignature 2>&1 | (grep -F -v 'will invalidate the code signature' || true))

# Copy helpers.
add_custom_command(
	COMMENT "Copying compiler CLIs into Vuo.framework"
	COMMAND cp -a
		${CONAN_LLVM_ROOT}/bin/clang
		${CONAN_LLVM_ROOT}/bin/llvm-link
		${PROJECT_BINARY_DIR}/bin/install_name_tool
		${PROJECT_BINARY_DIR}/bin/ld
		${PROJECT_BINARY_DIR}/bin/lipo
		${PROJECT_SOURCE_DIR}/base/codesignWrapper.sh
		"${frameworkVersionPath}/Helpers"

	COMMAND install_name_tool -rpath @loader_path/../lib @loader_path/../Modules ${frameworkVersionPath}/Helpers/clang ${installNameToolIgnoreSignature}
	COMMAND install_name_tool -add_rpath @loader_path/../../../.. ${frameworkVersionPath}/Helpers/clang ${installNameToolIgnoreSignature}
	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Helpers/clang ${codesignAdHocIgnoreReplace}

	COMMAND install_name_tool -rpath @loader_path/../lib @loader_path/../Modules ${frameworkVersionPath}/Helpers/llvm-link ${installNameToolIgnoreSignature}
	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Helpers/llvm-link ${codesignAdHocIgnoreReplace}

	COMMAND install_name_tool -rpath @executable_path/../lib/ @loader_path/../Modules ${frameworkVersionPath}/Helpers/ld ${installNameToolIgnoreSignature}
	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Helpers/ld ${codesignAdHocIgnoreReplace}

	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Helpers/codesignWrapper.sh

	OUTPUT "${frameworkVersionPath}/Helpers/clang"
)
add_custom_target(VuoFrameworkCompilerCLI DEPENDS "${frameworkVersionPath}/Helpers/clang")
add_dependencies(VuoFramework VuoFrameworkCompilerCLI)


file(MAKE_DIRECTORY ${frameworkVersionPath})
file(CREATE_LINK "Versions/Current/Documentation" "${frameworkPath}/Documentation" SYMBOLIC)
file(CREATE_LINK "Versions/Current/Frameworks"    "${frameworkPath}/Frameworks"    SYMBOLIC)
file(CREATE_LINK "Versions/Current/Headers"       "${frameworkPath}/Headers"       SYMBOLIC)
file(CREATE_LINK "Versions/Current/Helpers"       "${frameworkPath}/Helpers"       SYMBOLIC)
file(CREATE_LINK "Versions/Current/Modules"       "${frameworkPath}/Modules"       SYMBOLIC)
file(CREATE_LINK "Versions/Current/SDKs"          "${frameworkPath}/SDKs"          SYMBOLIC)

file(MAKE_DIRECTORY "${frameworkVersionPath}/Helpers")
file(CREATE_LINK "clang" "${frameworkVersionPath}/Helpers/clang++" SYMBOLIC)

file(MAKE_DIRECTORY "${frameworkVersionPath}/Helpers/ModuleCompiler")


# Copy VuoCompositionLoader.app into Vuo.framework.
set(frameworkCompositionLoader "${frameworkVersionPath}/Helpers/VuoCompositionLoader.app/Contents/MacOS/VuoCompositionLoader")
add_custom_command(
	DEPENDS
		VuoCompositionLoader
		"${PROJECT_BINARY_DIR}/bin/VuoCompositionLoader.app/Contents/Info.plist"
		"${PROJECT_BINARY_DIR}/bin/VuoCompositionLoader.app/Contents/MacOS/VuoCompositionLoader"
	COMMENT "Copying VuoCompositionLoader into Vuo.framework"

	COMMAND /usr/bin/rsync --recursive --links --perms
		"${PROJECT_BINARY_DIR}/bin/VuoCompositionLoader.app"
		"${frameworkVersionPath}/Helpers"

	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Helpers/VuoCompositionLoader.app ${codesignAdHocIgnoreReplace}

	OUTPUT
		${frameworkVersionPath}/Helpers/VuoCompositionLoader.app/Contents/Info.plist
		${frameworkCompositionLoader}
)
add_custom_target(VuoFrameworkCompositionLoader DEPENDS ${frameworkCompositionLoader})
add_dependencies(VuoFramework VuoFrameworkCompositionLoader)
list(APPEND generatedFiles
	${frameworkCompositionLoader}
)


# Copy node sets into Vuo.framework.
get_property(VuoNodeSets GLOBAL PROPERTY VuoNodeSets)
foreach (nodeSet ${VuoNodeSets})
	list(APPEND nodeSetArchives "${PROJECT_BINARY_DIR}/node/${nodeSet}/${nodeSet}.vuonode")
	list(APPEND nodeSetArchivesDest "${frameworkVersionPath}/Modules/${nodeSet}.vuonode")
endforeach()
add_custom_command(
	DEPENDS
		${VuoNodeSets} ${nodeSetArchives}
	COMMENT "Copying node sets into Vuo.framework"
	COMMAND /usr/bin/rsync ${nodeSetArchives} "${frameworkVersionPath}/Modules"
	OUTPUT ${nodeSetArchivesDest}
)
add_custom_target(VuoFrameworkNodeSets DEPENDS ${nodeSetArchivesDest})
add_dependencies(VuoFramework VuoFrameworkNodeSets)
list(APPEND generatedFiles
	${nodeSetArchivesDest}
)


# Copy dependency frameworks into Vuo.framework.
get_target_property(VuoClangCFrameworkGeneratedFiles VuoClangCFramework VuoGeneratedFiles)
get_target_property(VuoClangFrameworkGeneratedFiles VuoClangFramework VuoGeneratedFiles)
get_target_property(VuoLLVMCFrameworkGeneratedFiles VuoLLVMCFramework VuoGeneratedFiles)
get_target_property(VuoLLVMFrameworkGeneratedFiles  VuoLLVMFramework  VuoGeneratedFiles)
set(dependencyFrameworksDest
	${frameworkVersionPath}/Frameworks/clang-c.framework/Versions/Current/clang-c
	${frameworkVersionPath}/Frameworks/clang.framework/Versions/Current/clang
	${frameworkVersionPath}/Frameworks/llvm-c.framework/Versions/Current/llvm-c
	${frameworkVersionPath}/Frameworks/llvm.framework/Versions/Current/llvm
	${frameworkVersionPath}/Frameworks/FxPlug.framework/Versions/Current/FxPlug
	${frameworkVersionPath}/Frameworks/PluginManager.framework/Versions/Current/PluginManager
	${frameworkVersionPath}/Frameworks/Syphon.framework/Versions/Current/Syphon
	${frameworkVersionPath}/Frameworks/HapInAVFoundation.framework/Versions/Current/HapInAVFoundation
)
add_custom_command(
	DEPENDS
		# Work around apparent CMake bug where it generates nonexistent Ninja targets for macOS frameworks.
		${VuoClangCFrameworkGeneratedFiles} # VuoClangCFramework
		${VuoClangFrameworkGeneratedFiles} # VuoClangFramework
		${VuoLLVMCFrameworkGeneratedFiles} # VuoLLVMCFramework
		${VuoLLVMFrameworkGeneratedFiles}  # VuoLLVMFramework
	COMMENT "Copying dependency frameworks into Vuo.framework"
	COMMAND /usr/bin/rsync --recursive --links --perms
		${PROJECT_BINARY_DIR}/lib/clang-c.framework
		${PROJECT_BINARY_DIR}/lib/clang.framework
		${PROJECT_BINARY_DIR}/lib/llvm-c.framework
		${PROJECT_BINARY_DIR}/lib/llvm.framework
		${CONAN_FXPLUG_ROOT}/Library/Developer/Frameworks/FxPlug.framework
		${CONAN_FXPLUG_ROOT}/Library/Developer/Frameworks/PluginManager.framework
		${CONAN_HAP_ROOT}/lib/HapInAVFoundation.framework
		${CONAN_SYPHON_ROOT}/lib/Syphon.framework
		${frameworkVersionPath}/Frameworks
	COMMAND chmod 644 ${frameworkVersionPath}/Frameworks/HapInAVFoundation.framework/Versions/Current/Frameworks/*.dylib

	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Frameworks/clang-c.framework  ${codesignAdHocIgnoreReplace}
	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Frameworks/clang.framework  ${codesignAdHocIgnoreReplace}
	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Frameworks/llvm-c.framework ${codesignAdHocIgnoreReplace}
	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Frameworks/llvm.framework   ${codesignAdHocIgnoreReplace}

	COMMAND install_name_tool -id @rpath/Vuo.framework/Versions/${VuoFrameworkVersion}/Frameworks/Syphon.framework/Versions/A/Syphon ${frameworkVersionPath}/Frameworks/Syphon.framework/Versions/A/Syphon ${installNameToolIgnoreSignature}
	COMMAND ${codesignAdHoc} --force "${frameworkVersionPath}/Frameworks/Syphon.framework" ${codesignAdHocIgnoreReplace}

	# Update timestamps, to help `make` believe this target was successfully rebuilt.
	COMMAND touch ${dependencyFrameworksDest}

	OUTPUT ${dependencyFrameworksDest}
)
add_custom_target(VuoFrameworkDependencyFrameworks DEPENDS ${dependencyFrameworksDest})
add_dependencies(VuoFrameworkDylib VuoFrameworkDependencyFrameworks)
add_dependencies(VuoFramework VuoFrameworkDependencyFrameworks)
list(APPEND generatedFiles
	${dependencyFrameworksDest}
)


# Copy core types and libraries into Vuo.framework.
function (VuoGetObjects outputObjectsVariable outputDestVariable target binaryDir)
	get_target_property(objects ${target} SOURCES)
	list(TRANSFORM objects PREPEND ${binaryDir})
	set(${outputObjectsVariable} "${objects}" PARENT_SCOPE)

	get_target_property(objects ${target} SOURCES)
	list(TRANSFORM objects PREPEND ${frameworkVersionPath}/Modules/)
	set(${outputDestVariable} "${objects}" PARENT_SCOPE)
endfunction()
VuoGetObjects(coreTypeObjects            coreTypeObjectsDest           vuo.core.types              "${PROJECT_BINARY_DIR}/type/")
VuoGetObjects(coreLibraryObjects         coreLibraryObjectsDest        vuo.core.libraries          "${PROJECT_BINARY_DIR}/library/")
VuoGetObjects(coreModuleLibraryObjects   coreModuleLibraryObjectsDest  vuo.core.module.libraries   "${PROJECT_BINARY_DIR}/library/")
VuoGetObjects(coreRuntimeObjects         coreRuntimeObjectsDest        vuo.core.runtime.objects    "${PROJECT_BINARY_DIR}/runtime/")
list(FILTER coreTypeObjects              EXCLUDE REGEX "\.hh?$")
list(FILTER coreTypeObjectsDest          EXCLUDE REGEX "\.hh?$")
list(FILTER coreLibraryObjects           EXCLUDE REGEX "\.hh?$")
list(FILTER coreLibraryObjectsDest       EXCLUDE REGEX "\.hh?$")
list(FILTER coreModuleLibraryObjects     EXCLUDE REGEX "\.hh?$")
list(FILTER coreModuleLibraryObjectsDest EXCLUDE REGEX "\.hh?$")
list(FILTER coreRuntimeObjects           EXCLUDE REGEX "\.(bc|hh?)$")
list(FILTER coreRuntimeObjectsDest       EXCLUDE REGEX "\.(bc|hh?)$")
set(frameworkCoreDylibsDest
	${frameworkVersionPath}/Modules/libVuoApp.dylib
	${frameworkVersionPath}/Modules/libVuoCompositionStub.dylib
	${frameworkVersionPath}/Modules/libVuoGlContext.dylib
	${frameworkVersionPath}/Modules/libVuoGlPool.dylib
	${frameworkVersionPath}/Modules/libVuoHeap.dylib
)
set(frameworkCoreOutputs
	${coreTypeObjectsDest}
	${coreListTypeObjectsDest}
	${coreLibraryObjectsDest}
	${coreModuleLibraryObjectsDest}
	${coreRuntimeObjectsDest}
	${frameworkCoreDylibsDest}
)
add_custom_command(
	DEPENDS
		vuo.core.types
		vuo.core.types.compound
		vuo.core.libraries
		vuo.core.module.libraries
		vuo.core.runtime.objects
		VuoApp
		VuoCompositionStub
		VuoGlContext
		VuoGlPool
		VuoHeap
	COMMENT "Copying Vuo core types and libraries into Vuo.framework"
	# Use `--extended-attributes` since `codesign` stores signatures for `.bc` and `.o` files
	# in macOS extended attributes (a.k.a. xattr, resource forks),
	# and those code signatures are required for Notarization.
	COMMAND /usr/bin/rsync --extended-attributes
		${coreTypeObjects}
		${coreListTypeObjects}
		${coreLibraryObjects}
		${coreModuleLibraryObjects}
		${coreRuntimeObjects}
		"${PROJECT_BINARY_DIR}/lib/libVuoApp.dylib"
		"${PROJECT_BINARY_DIR}/lib/libVuoCompositionStub.dylib"
		"${PROJECT_BINARY_DIR}/lib/libVuoGlContext.dylib"
		"${PROJECT_BINARY_DIR}/lib/libVuoGlPool.dylib"
		"${PROJECT_BINARY_DIR}/lib/libVuoHeap.dylib"
		${frameworkVersionPath}/Modules
	COMMAND chmod 644 ${frameworkCoreDylibsDest}
	COMMAND install_name_tool -id @rpath/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules/libVuoApp.dylib       ${frameworkVersionPath}/Modules/libVuoApp.dylib       ${installNameToolIgnoreSignature}
	COMMAND install_name_tool -id @rpath/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules/libVuoGlContext.dylib ${frameworkVersionPath}/Modules/libVuoGlContext.dylib ${installNameToolIgnoreSignature}
	COMMAND install_name_tool -id @rpath/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules/libVuoGlPool.dylib    ${frameworkVersionPath}/Modules/libVuoGlPool.dylib    ${installNameToolIgnoreSignature}
	COMMAND install_name_tool -id @rpath/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules/libVuoHeap.dylib      ${frameworkVersionPath}/Modules/libVuoHeap.dylib      ${installNameToolIgnoreSignature}
	COMMAND install_name_tool -add_rpath @loader_path ${frameworkVersionPath}/Modules/libVuoApp.dylib       ${installNameToolIgnoreSignature}
	COMMAND install_name_tool -add_rpath @loader_path ${frameworkVersionPath}/Modules/libVuoGlContext.dylib ${installNameToolIgnoreSignature}
	COMMAND install_name_tool -add_rpath @loader_path ${frameworkVersionPath}/Modules/libVuoGlPool.dylib    ${installNameToolIgnoreSignature}
	COMMAND ${codesignAdHoc} ${frameworkCoreDylibsDest} ${codesignAdHocIgnoreReplace}
	OUTPUT ${frameworkCoreOutputs}
)
add_custom_target(VuoFrameworkCore DEPENDS ${frameworkCoreOutputs})
add_dependencies(VuoFrameworkDylib VuoFrameworkCore)
list(APPEND generatedFiles
	${frameworkCoreOutputs}
)


if (VuoPro)
	include(pro/pro.cmake)
endif()


# Copy module-compilers into Vuo.framework.
get_target_property(moduleCompilers VuoModuleCompilers VuoGeneratedFiles)
foreach (moduleCompiler ${moduleCompilers})
	get_filename_component(moduleCompiler ${moduleCompiler} NAME)
	list(APPEND moduleCompilersDest "${frameworkVersionPath}/Helpers/ModuleCompiler/${moduleCompiler}")
endforeach()
add_custom_command(
	DEPENDS
		VuoModuleCompilers ${moduleCompilers}
	COMMENT "Copying module-compilers into Vuo.framework"
	COMMAND /usr/bin/rsync
		${moduleCompilers}
		${frameworkVersionPath}/Helpers/ModuleCompiler
	COMMAND ${codesignAdHoc} ${moduleCompilersDest} ${codesignAdHocIgnoreReplace}
	COMMAND chmod 644 ${moduleCompilersDest}
	OUTPUT ${moduleCompilersDest}
)
add_custom_target(VuoFrameworkModuleCompilers DEPENDS ${moduleCompilersDest})
add_dependencies(VuoFramework
	VuoFrameworkModuleCompilers
	VuoModulePCH
)
get_target_property(precompiledHeaders VuoModulePCH VuoGeneratedFiles)
list(APPEND generatedFiles
	${moduleCompilersDest}
	${precompiledHeaders}
)


# Copy dependency libraries into Vuo.framework.
set(conanDependencies "${CONAN_DEPENDENCIES}")
# We don't need to copy the WJElement libs, since we're only using them for testing.
list(REMOVE_ITEM conanDependencies wjelement)
# Exclude OpenSSL (for US export compliance).
list(REMOVE_ITEM conanDependencies openssl)
# Exclude frameworks since they're copied by VuoFrameworkDependencyFrameworks.
list(REMOVE_ITEM conanDependencies fxplug)
list(REMOVE_ITEM conanDependencies hap)
list(REMOVE_ITEM conanDependencies syphon)
# Find the absolute path to each library.
foreach (conanDependency ${conanDependencies})
	string(TOUPPER ${conanDependency} conanDependency)
	set(libs "${CONAN_LIBS_${conanDependency}}")

	set(libsSource ${libs})
	list(TRANSFORM libsSource PREPEND "${CONAN_${conanDependency}_ROOT}/lib/lib")
	list(TRANSFORM libsSource APPEND ".dylib")
	list(APPEND conanLibraries "${libsSource}")

	set(libsDest ${libs})
	list(TRANSFORM libsDest PREPEND "${frameworkVersionPath}/Modules/lib")
	list(TRANSFORM libsDest APPEND ".dylib")
	list(APPEND conanLibrariesDest "${libsDest}")
endforeach()
list(APPEND conanLibraries
	${PROJECT_SOURCE_DIR}/node/vuo.leap/Leap/libLeap.dylib
	${CONAN_LD64_ROOT}/lib/libtapi.dylib
	# We need to copy all libfacedetection versions (they aren't included in CONAN_LIBS since we need to choose at runtime which of the x86 variants to use).
	${CONAN_LIBFACEDETECTION_ROOT}/lib/libfacedetection_x86.dylib
	${CONAN_LIBFACEDETECTION_ROOT}/lib/libfacedetection_x86avx2.dylib
	${CONAN_LIBFACEDETECTION_ROOT}/lib/libfacedetection_arm.dylib
)
list(APPEND conanLibrariesDest
	${frameworkVersionPath}/Modules/libLeap.dylib
	${frameworkVersionPath}/Modules/libfacedetection_x86.dylib
	${frameworkVersionPath}/Modules/libfacedetection_x86avx2.dylib
	${frameworkVersionPath}/Modules/libfacedetection_arm.dylib
)
# Copy.
add_custom_command(
	COMMENT "Copying dependency libraries into Vuo.framework"
	COMMAND /usr/bin/rsync
		${conanLibraries}
		${frameworkVersionPath}/Modules
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/fixRpathsConan.sh
		${PROJECT_SOURCE_DIR}
		Vuo.framework/Versions/${VuoFrameworkVersion}/Modules
		${conanLibrariesDest}
	COMMAND chmod 644 ${conanLibrariesDest}
	COMMAND install_name_tool -id @rpath/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules/libLeap.dylib ${frameworkVersionPath}/Modules/libLeap.dylib ${installNameToolIgnoreSignature}
	COMMAND ${codesignAdHoc} ${frameworkVersionPath}/Modules/libLeap.dylib ${codesignAdHocIgnoreReplace}

	OUTPUT ${conanLibrariesDest}
)
add_custom_target(VuoFrameworkConanLibraries DEPENDS ${conanLibrariesDest})
add_dependencies(VuoFrameworkDylib VuoFrameworkConanLibraries)
list(APPEND generatedFiles
	${conanLibrariesDest}
)


# Copy dependency headers into Vuo.framework.
file(MAKE_DIRECTORY "${frameworkVersionPath}/Headers")
file(COPY ${CONAN_JSONC_ROOT}/include/json-c       DESTINATION "${frameworkVersionPath}/Headers")
file(COPY ${CONAN_ZEROMQ_ROOT}/include/zmq         DESTINATION "${frameworkVersionPath}/Headers")


# Copy macOS system headers into Vuo.framework.
set(macosHeadersDestination "${frameworkVersionPath}/Headers/macos")
set(rsyncHeaders /usr/bin/rsync --recursive --links --perms --include '*.h' --exclude '*')
add_custom_command(
	COMMENT "Copying macOS headers into Vuo.framework"
	COMMAND mkdir -p ${macosHeadersDestination}
	COMMAND /usr/bin/rsync --recursive --links --perms
		--exclude '*curses*'
		--exclude '/sched.h'
		--exclude 'tcl*'
		--exclude 'tk*'
		--include '*.h'
		--include _types
		--include _pthread
		--include architecture
		--include arm
		--include bsm
		--include device
		--include dispatch
		--include i386
		--include libkern
		--include mach
		--include mach_debug
		--include machine
		--include malloc
		--include objc
		--include os
		--include pthread
		--include secure
		--include sys
		--include uuid
		--include xlocale
		--include xpc
		--exclude '*'
		${CMAKE_OSX_SYSROOT}/usr/include/
		${macosHeadersDestination}
	COMMAND ${rsyncHeaders} ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/CoreFoundation.framework/Versions/A/Headers/ ${macosHeadersDestination}/CoreFoundation
	COMMAND ${rsyncHeaders} ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/IOKit.framework/Versions/A/Headers/          ${macosHeadersDestination}/IOKit
	COMMAND ${rsyncHeaders} ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/IOSurface.framework/Versions/A/Headers/      ${macosHeadersDestination}/IOSurface
	COMMAND ${rsyncHeaders} ${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/OpenGL.framework/Versions/A/Headers/         ${macosHeadersDestination}/OpenGL
	OUTPUT ${macosHeadersDestination}/AvailabilityMacros.h
)
add_custom_target(VuoFrameworkMacOSHeaders DEPENDS ${macosHeadersDestination}/AvailabilityMacros.h)
add_dependencies(VuoFrameworkDylib VuoFrameworkMacOSHeaders)


# Copy macOS SDK text-based dylib stubs into Vuo.framework,
# so we can link on platforms that don't include system dylibs (macOS 11).
set(macOSSDKDestination "${frameworkVersionPath}/SDKs/MacOSX.sdk")
add_custom_command(
	COMMENT "Copying macOS SDK TBDs into Vuo.framework"
	WORKING_DIRECTORY ${CONAN_MACOS-SDK_ROOT}
	COMMAND mkdir -p ${macOSSDKDestination}
	COMMAND /usr/bin/rsync
		--relative
		--recursive
		--links
		--exclude '**/Headers'
		--exclude '**/Modules'
		--exclude '*.app'
		--exclude '*.map'
		--exclude '*.modulemap'
		--exclude 'libLLVMContainer.tbd'
		# Exclude ATS since it contains symlinks to TBDs in PrivateFrameworks,
		# but we don't (currently) need to copy those private TBDs into Vuo.framework.
		--exclude 'ATS.framework'
		System/Library/Frameworks/AVFoundation.framework
		System/Library/Frameworks/Accelerate.framework
		System/Library/Frameworks/AppKit.framework
		System/Library/Frameworks/ApplicationServices.framework
		System/Library/Frameworks/AudioToolbox.framework
		System/Library/Frameworks/CFNetwork.framework
		System/Library/Frameworks/Carbon.framework
		System/Library/Frameworks/ColorSync.framework
		System/Library/Frameworks/CoreAudio.framework
		System/Library/Frameworks/CoreFoundation.framework
		System/Library/Frameworks/CoreGraphics.framework
		System/Library/Frameworks/CoreImage.framework
		System/Library/Frameworks/CoreMIDI.framework
		System/Library/Frameworks/CoreMedia.framework
		System/Library/Frameworks/CoreMediaIO.framework
		System/Library/Frameworks/CoreServices.framework
		System/Library/Frameworks/CoreText.framework
		System/Library/Frameworks/CoreVideo.framework
		System/Library/Frameworks/Foundation.framework
		System/Library/Frameworks/IOKit.framework
		System/Library/Frameworks/IOSurface.framework
		System/Library/Frameworks/ImageIO.framework
		System/Library/Frameworks/Metal.framework
		System/Library/Frameworks/OpenGL.framework
		System/Library/Frameworks/PDFKit.framework
		System/Library/Frameworks/Quartz.framework
		System/Library/Frameworks/QuartzCore.framework
		System/Library/Frameworks/QuickLook.framework
		System/Library/Frameworks/VideoToolbox.framework
		System/Library/Frameworks/WebKit.framework
		System/Library/PrivateFrameworks/UIFoundation.framework
		usr/lib/libSystem.tbd
		usr/lib/libSystem.B.tbd
		usr/lib/libc++.tbd
		usr/lib/libc++.1.tbd
		usr/lib/libextension.tbd
		usr/lib/libobjc.tbd
		usr/lib/libobjc.A.tbd
		usr/lib/system/*.tbd
		${macOSSDKDestination}
	OUTPUT ${macOSSDKDestination}/usr/lib/libSystem.tbd
)
add_custom_target(VuoFrameworkMacOSSDK DEPENDS ${macOSSDKDestination}/usr/lib/libSystem.tbd)
add_dependencies(VuoFrameworkDylib VuoFrameworkMacOSSDK)


# Copy source-tree and generated headers into Vuo.framework.
file(GLOB frameworkHeaders
	../base/*.h
	../base/*.hh
	../common/*.h
	../compiler/*.h
	../compiler/*.hh
	../library/*.h
	../library/*.hh
	../node/*.h
	../node/*/*.h
	../node/*/*.hh
	../runner/*.h
	../runner/*.hh
	../runtime/*.h
	../runtime/*.hh
)

get_directory_property(coreTypeHeaders DIRECTORY ../type DEFINITION VuoCoreTypeHeaders)
list(TRANSFORM coreTypeHeaders PREPEND "${PROJECT_SOURCE_DIR}/type/")
get_directory_property(coreTypeHeadersDest DIRECTORY ../type DEFINITION VuoCoreTypeHeaders)
list(TRANSFORM coreTypeHeadersDest PREPEND "${frameworkVersionPath}/Headers/")

list(APPEND generatedTypeHeaderTargets vuo.core.types.compound.headers)
get_target_property(generatedTypeHeadersInTarget vuo.core.types.compound.headers VuoGeneratedFiles)
list(APPEND generatedTypeHeaders ${generatedTypeHeadersInTarget})
foreach (nodeSet ${VuoNodeSets})
	if (TARGET "${nodeSet}.types.compound.headers")
		list(APPEND generatedTypeHeaderTargets ${nodeSet}.types.compound.headers)
		get_target_property(generatedTypeHeadersInTarget ${nodeSet}.types.compound.headers VuoGeneratedFiles)
		list(APPEND generatedTypeHeaders ${generatedTypeHeadersInTarget})
	endif()
endforeach()

set(frameworkHeadersOutputs
	${frameworkVersionPath}/Headers/VuoConfig.h
	${frameworkVersionPath}/Headers/coreTypes.h
	${frameworkVersionPath}/Headers/module.h
	${frameworkVersionPath}/Headers/node.h
	${frameworkVersionPath}/Headers/type.h
	${coreTypeHeadersDest}
)
add_custom_command(
	DEPENDS
		${PROJECT_BINARY_DIR}/base/VuoConfig-for-framework.h
		VuoCoreTypesHeader
		${PROJECT_SOURCE_DIR}/type/type.h
		${coreTypeHeaders}
		${generatedTypeHeaders}
		${frameworkHeaders}
	COMMENT "Copying Vuo headers into Vuo.framework"
	COMMAND cp ${PROJECT_BINARY_DIR}/base/VuoConfig-for-framework.h ${frameworkVersionPath}/Headers/VuoConfig.h
	COMMAND cp
		${PROJECT_BINARY_DIR}/base/config/coreTypes.h
		${PROJECT_SOURCE_DIR}/type/type.h
		${coreTypeHeaders}
		${generatedTypeHeaders}
		${frameworkHeaders}
		${frameworkVersionPath}/Headers
	OUTPUT ${frameworkHeadersOutputs}
)
add_custom_target(VuoFrameworkHeaders
	DEPENDS
		${frameworkHeadersOutputs}
		${coreTypeHeaders}
		${frameworkHeaders}
		${generatedTypeHeaderTargets}
)
add_dependencies(VuoFrameworkDylib VuoFrameworkHeaders)
list(APPEND generatedFiles
	${frameworkHeadersOutputs}
	${coreTypeHeaders}
	${frameworkHeaders}
)


# Copy generic compound type headers and sources into Vuo.framework.

file(GLOB compoundTypeSources RELATIVE "${PROJECT_SOURCE_DIR}/type/compound"
	"${PROJECT_SOURCE_DIR}/type/compound/*.h"
	"${PROJECT_SOURCE_DIR}/type/compound/*.cc"
)
set(compoundTypeSourcesIn ${compoundTypeSources})
set(compoundTypeSourcesOut ${compoundTypeSources})
list(TRANSFORM compoundTypeSourcesIn PREPEND "${PROJECT_SOURCE_DIR}/type/compound/")
list(TRANSFORM compoundTypeSourcesOut PREPEND "${frameworkVersionPath}/Modules/")
add_custom_command(
	DEPENDS ${compoundTypeSourcesIn}
	COMMAND /usr/bin/rsync ${compoundTypeSourcesIn} "${frameworkVersionPath}/Modules"

	# Update timestamps, to help `make` believe this target was successfully rebuilt.
	COMMAND touch ${compoundTypeSourcesOut}

	OUTPUT ${compoundTypeSourcesOut}
)
add_custom_target(VuoFrameworkGenericCompoundTypes DEPENDS ${compoundTypeSourcesOut})
set_property(TARGET VuoFrameworkGenericCompoundTypes APPEND PROPERTY VuoGeneratedFiles "${compoundTypeSourcesOut}")
add_dependencies(VuoFrameworkDylib VuoFrameworkGenericCompoundTypes)


# Generate the framework's master header.
file(GLOB frameworkMasterHeaders
	../base/*.h
	../base/*.hh
	../common/*.h
	../compiler/*.h
	../compiler/*.hh
	../library/*.h
	../library/*.hh
	../node/*.h
	../runner/*.h
	../runner/*.hh
	../runtime/*.h
	../runtime/*.hh
	../type/*.h
)
list(TRANSFORM frameworkMasterHeaders REPLACE "^.*/" "")
list(FILTER frameworkMasterHeaders EXCLUDE REGEX "^(VuoGraphics(Layer|View|Window|WindowDelegate|WindowDrag)|VuoImageResize|VuoMathExpression|VuoShader(Shaders|Uniforms)|VuoSmooth|VuoUrlParser|VuoWindowRecorder|VuoWindowTextInternal|coreTypes)\.h+$")
list(SORT frameworkMasterHeaders)
add_custom_command(
	DEPENDS
		${PROJECT_SOURCE_DIR}/framework/Vuo.stub.h
	COMMENT "Generating Vuo.h"
	WORKING_DIRECTORY ${frameworkVersionPath}/Headers
	COMMAND LC_ALL=C ${PROJECT_SOURCE_DIR}/framework/generateFrameworkHeader.pl ${PROJECT_SOURCE_DIR}/framework/Vuo.stub.h ${frameworkMasterHeaders} > Vuo.h
	OUTPUT ${frameworkVersionPath}/Headers/Vuo.h
)
add_custom_target(VuoFrameworkMasterHeader DEPENDS ${frameworkVersionPath}/Headers/Vuo.h)
target_sources(VuoFrameworkMasterHeader PRIVATE Vuo.stub.h)
add_dependencies(VuoFrameworkDylib VuoFrameworkMasterHeader)
list(APPEND generatedFiles
	${frameworkVersionPath}/Headers/Vuo.h
)


file(GLOB licenses
	${PROJECT_SOURCE_DIR}/license/*
	${PROJECT_BINARY_DIR}/license/*
)
VuoBundleFiles(VuoFrameworkDylib "Documentation/Licenses" ${licenses})


# Create symlinks within the Headers directory so that the module PCH files can use it as their sysroot.
add_custom_command(
	DEPENDS
		VuoFrameworkDependencyFrameworks
		VuoFrameworkMacOSHeaders
		VuoFrameworkHeaders
	COMMENT "Creating symlinks for headers"
	COMMAND mkdir -p "${frameworkVersionPath}/Headers/usr"
	COMMAND rm -f "${frameworkVersionPath}/Headers/usr/include"
				  "${frameworkVersionPath}/Headers/macos/c++"
				  "${frameworkVersionPath}/Headers/clang"
				  "${frameworkVersionPath}/Headers/llvm"
	COMMAND ln -s "../macos" "${frameworkVersionPath}/Headers/usr/include"
	COMMAND ln -s "../../Frameworks/llvm.framework/Versions/A/Headers/lib/c++" "${frameworkVersionPath}/Headers/macos/c++"
	COMMAND ln -s "../Frameworks/llvm.framework/Versions/Current/lib/clang/5.0.2" "${frameworkVersionPath}/Headers/clang"
	COMMAND ln -s "../Frameworks/llvm.framework/Versions/Current/Headers/lib" "${frameworkVersionPath}/Headers/llvm"
	OUTPUT "${frameworkVersionPath}/Headers/usr/include"
)
add_custom_target(ModulePCHSysroot DEPENDS "${frameworkVersionPath}/Headers/usr/include")


# Generate the built-in module caches in the Modules directory.
# (Do this after the rest of Vuo.framework is created so the cache dylibs will refer to the install_name_tool-ed dylibs in Vuo.framework.)
set(frameworkCacheDir ${frameworkVersionPath}/Modules/Builtin)
set(frameworkCache ${frameworkCacheDir}/libVuoModuleCache.dylib)
if (VUO_COMPILER_DEVELOPER)
	set(dependsOnVuoCompile "")
else()
	set(dependsOnVuoCompile "vuo-compile" VuoCModuleCompiler)
endif()
get_property(VuoCacheableObjects GLOBAL PROPERTY VuoCacheableObjects)

set(frameworkCacheParts "")
list(LENGTH CMAKE_OSX_ARCHITECTURES archCount)
foreach (arch ${CMAKE_OSX_ARCHITECTURES})
	set(frameworkCachePart ${frameworkCacheDir}/libVuoModuleCache.dylib-${arch})
	set(frameworkCachePartBuild ${PROJECT_BINARY_DIR}/lib/libVuoModuleCache.dylib-${arch})
	set(dependsOn
		${dependsOnVuoCompile}
		VuoFrameworkCompilerCLI
		VuoFrameworkDependencyFrameworks
		VuoFrameworkHeaders
		VuoFrameworkNodeSets
		VuoFrameworkHeaders
		VuoFrameworkCore
		VuoFrameworkConanLibraries
		vuo.core.libraries
		vuo.core.module.libraries
		vuo.core.runtime.objects
		vuo.core.types
		${generatedModuleTargets}
		${VuoCacheableObjects}
	)
	if (VUO_NODE_DEVELOPER)
		add_custom_command(
			DEPENDS ${dependsOn}
			COMMENT "Generating built-in module caches (${arch})"
			COMMAND rm -f ${frameworkCachePart} ${frameworkCachePartBuild}
			COMMAND ${PROJECT_BINARY_DIR}/bin/vuo-compile --target ${arch}-apple-macosx10.10.0 --generate-builtin-modules "${frameworkPath}"
			COMMAND touch ${frameworkCachePartBuild}
			OUTPUT ${frameworkCachePartBuild}
		)
	else()
		add_custom_command(
			DEPENDS ${dependsOn}
			COMMENT "Generating built-in module caches (${arch})"
			COMMAND rm -f ${frameworkCachePart}
			COMMAND ${PROJECT_BINARY_DIR}/bin/vuo-compile --target ${arch}-apple-macosx10.10.0 --generate-builtin-module-caches "${frameworkPath}"
			COMMAND install_name_tool -id @rpath/Vuo.framework/Versions/${VuoFrameworkVersion}/Modules/Builtin/libVuoModuleCache.dylib ${frameworkCachePart}
			COMMAND install_name_tool -rpath ${frameworkPath}/.. @loader_path/../../../../.. ${frameworkCachePart}
			COMMAND ${codesignAdHoc} ${frameworkCachePart}
			COMMAND chmod 644 ${frameworkCachePart}
			# vuo-compile generates the single-arch part inside the framework;
			# move it aside so that only the final merged dylib is in the framework.
			COMMAND mv ${frameworkCachePart} ${frameworkCachePartBuild}
			COMMAND mv ${frameworkCacheDir}/manifest.txt-${arch} ${PROJECT_BINARY_DIR}/lib/manifest.txt-${arch}
			OUTPUT ${frameworkCachePartBuild}
		)
	endif()
	list(APPEND frameworkCacheParts ${frameworkCachePartBuild})
endforeach()

if (VUO_NODE_DEVELOPER)
	add_custom_command(
		DEPENDS ${frameworkCacheParts}
		COMMAND rm -f ${frameworkCache}
		COMMAND touch ${frameworkCache}
		OUTPUT ${frameworkCache}
	)
else()
	if (archCount GREATER 1)
		# The manifest files should be the same; pick one to include.
		list(GET CMAKE_OSX_ARCHITECTURES 0 arch)
		add_custom_command(
			DEPENDS ${frameworkCacheParts}
			COMMENT "Merging built-in module cache"
			COMMAND lipo -create ${frameworkCacheParts} -output ${frameworkCache}
			COMMAND cp ${PROJECT_BINARY_DIR}/lib/manifest.txt-${arch} ${frameworkCacheDir}/manifest.txt
			OUTPUT ${frameworkCache}
		)
	else()
		add_custom_command(
			DEPENDS ${frameworkCacheParts}
			COMMENT "Copying built-in module caches into framework"
			COMMAND cp ${frameworkCacheParts} ${frameworkCache}
			COMMAND cp ${PROJECT_BINARY_DIR}/lib/manifest.txt-* ${frameworkCacheDir}/manifest.txt
			OUTPUT ${frameworkCache}
		)
	endif()
endif()

add_custom_target(VuoFrameworkCache
	DEPENDS
		${frameworkCache}
		VuoFrameworkCompilerCLI
)
list(APPEND generatedFiles
	${frameworkCache}
)
add_dependencies(VuoFramework VuoFrameworkCache)


# Mark bundled resources as generated files,
# so Vuo.framework gets re-copied into Vuo.app if they change.
get_target_property(frameworkDylibSources VuoFrameworkDylib SOURCES)
foreach (source ${frameworkDylibSources})
	get_source_file_property(packageLocation ${source} MACOSX_PACKAGE_LOCATION)
	if (packageLocation)
		get_filename_component(file ${source} NAME)
		list(APPEND generatedFiles ${frameworkVersionPath}/${packageLocation}/${file})
	endif()
endforeach()

set_target_properties(VuoFramework PROPERTIES VuoGeneratedFiles "${generatedFiles}")


# Code-sign the whole mess after all the modules and frameworks and helpers have been copied into Vuo.framework.
add_custom_command(TARGET VuoFramework POST_BUILD
	# Save and restore VuoFrameworkDylib's timestamp, so that code-signing it
	# doesn't cause everything that depends on VuoFrameworkDylib to rebuild.
	COMMAND touch -r ${frameworkVersionPath}/Vuo ${frameworkPath}-dylib-timestamp
	COMMAND ${codesignAdHocVerify} ${frameworkVersionPath}/Vuo >/dev/null 2>&1 || ${codesignAdHoc} ${frameworkVersionPath}/Vuo ${codesignAdHocIgnoreReplace}
	COMMAND touch -r ${frameworkPath}-dylib-timestamp ${frameworkVersionPath}/Vuo
)
