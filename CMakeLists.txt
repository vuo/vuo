cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
cmake_policy(VERSION 3.16...3.20)


# Configuration ==============================================================
project(Vuo VERSION 2.3.1 LANGUAGES NONE)

# The major-version component of ${Vuo_VERSION}.
# Used in framework paths (e.g., `Vuo.framework/Versions/2/Vuo`).
set(VuoFrameworkVersion 2)

if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
	set(VUO_ADHOC_CODESIGN_DEFAULT ON)
else()
	set(VUO_ADHOC_CODESIGN_DEFAULT OFF)
endif()

option(VUO_COMPILER_DEVELOPER "Compiler development mode.  Allows you to make changes to VuoBase and VuoCompiler without rebuilding all nodes and types (but you're responsible for keeping track of whether nodes and types need to be rebuilt).  Saves several minutes per edit-compile-test cycle." OFF)
option(VUO_NODE_DEVELOPER     "Node development mode.  Allows you to make changes to nodes, types, and libraries without rebuilding the Vuo.framework cache.  Saves about 30 seconds per edit-compile-test cycle (but compositions take longer to start up initially)." OFF)
option(VUO_HEAP_TRACE         "Logs reference-counting errors to stderr (in addition to the usual telemetry-based error reporting), including backtraces.  Also enables using VuoHeap_addTrace().")
option(VUO_HEAP_TRACE_ALL     "Logs all reference-counting operations.  Very noisy.")
option(VUO_PROFILE            "Add instrumentation for performance profiling." OFF)
option(VUO_ANALYZE            "Run Clang Analyzer and other time-consuming analysis tools." OFF)
option(VUO_ADHOC_CODESIGN     "If enabled, executables and dylibs will be ad-hoc code-signed during the normal build, to support running on ARM64/Apple Silicon.  If disabled, code-signing will only happen for the VuoPackage target (which isn't part of the normal build)." ${VUO_ADHOC_CODESIGN_DEFAULT})
option(VUO_DOCUMENTATION      "Whether to build the user manual (PDF, HTML, MacHelp), node documentation, and Doxygen documentation." OFF)
option(VUO_EXAMPLES           "Whether to build the example compositions." OFF)


# Commands ===================================================================
if (EXISTS /Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib)
   set(mainThreadChecker DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib)
endif()
add_custom_target(go COMMAND ${CMAKE_COMMAND} -E env ${mainThreadChecker} ${PROJECT_BINARY_DIR}/bin/Vuo.app/Contents/MacOS/Vuo USES_TERMINAL)


# Early checks ===============================================================
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "\nOnly build Vuo outside the source tree.  For example:\n    rm -Rf CMakeCache.txt CMakeFiles\n    mkdir build\n    cd build\n    cmake ..")
endif()


# Cache and package version identifier =======================================
# Changing `VUO_VERSION_AND_BUILD` or `VUO_GIT_COMMIT` causes
# the entire project to rebuild because they're used in `base/VuoConfig.h`.
# Store those values in the cache, so they don't change unintentionally,
# such as when making manual changes in a tree that was first built by Jenkins.
if (NOT VUO_VERSION_AND_BUILD)
	if (DEFINED ENV{BRANCH_NAME})
		if ("$ENV{BRANCH_NAME}" STREQUAL "main")
			set(id "${Vuo_VERSION}.$ENV{BUILD_ID}")     # e.g., `2.3.0.11546`
		else()
			set(id "$ENV{BRANCH_NAME}.$ENV{BUILD_ID}")  # e.g., `17996-jenkins-various.2`
			set(VUO_BRANCH_BUILD ON CACHE BOOL "True if this Jenkins build is from a Git branch other than `main`.")
		endif()

		execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE)
		set(VUO_GIT_COMMIT "${commit}" CACHE STRING "The Git commit hash for this Jenkins build")
	else()
		cmake_host_system_information(RESULT hostname QUERY HOSTNAME)
		string(REGEX REPLACE "\\..*$" "" hostname "${hostname}")
		get_filename_component(dir "${PROJECT_SOURCE_DIR}" NAME)
		set(id "${hostname}.${dir}")                    # e.g., `joku.vuo3`
	endif()
	set(VUO_VERSION_AND_BUILD "${id}" CACHE STRING "The Vuo cache/package version identifier")
endif()
message(STATUS "Vuo build: ${VUO_VERSION_AND_BUILD} ${VUO_GIT_COMMIT}")


# Conan ======================================================================
set(conanFile "${PROJECT_SOURCE_DIR}/conanfile.py")
set(conanBuildInfo "${PROJECT_BINARY_DIR}/conanbuildinfo.cmake")
set_directory_properties(PROPERTIES CMAKE_CONFIGURE_DEPENDS ${conanFile})
if (NOT EXISTS ${conanBuildInfo} OR ${conanFile} IS_NEWER_THAN ${conanBuildInfo})
	execute_process(COMMAND /usr/local/bin/conan install ${PROJECT_SOURCE_DIR} RESULT_VARIABLE conanStatus)
	if (${conanStatus})
		message(FATAL_ERROR "Couldn't install dependencies using Conan.")
	endif()
	file(TOUCH ${conanBuildInfo})
endif()

set(CMAKE_C_COMPILER_ID Clang)
set(CMAKE_C_COMPILER_VERSION 5.0.2)
set(CMAKE_CXX_COMPILER_ID ${CMAKE_C_COMPILER_ID})
set(CMAKE_CXX_COMPILER_VERSION ${CMAKE_C_COMPILER_VERSION})
set(CONAN_COMPILER clang)
set(CONAN_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
set(CONAN_CMAKE_SILENT_OUTPUT TRUE)
include("${conanBuildInfo}")
conan_basic_setup(KEEP_RPATHS TARGETS)

# Exclude unneeded LLVM/Clang libraries.
list(REMOVE_ITEM CONAN_LIBS_LLVM
	LLVMArchive
	LLVMDebugInfo
	LLVMMCJIT
	LLVMTableGen
	LTO
	clang
	clangTooling
)

# Exclude LLVM/Clang libraries distributed with macOS.
list(REMOVE_ITEM CONAN_LIBS_LLVM
	c++
	c++abi
)


# Compilers ==================================================================
include(CTest)

set(CMAKE_OSX_SYSROOT "${CONAN_MACOS-SDK_ROOT}" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.11 CACHE STRING "" FORCE)

execute_process(COMMAND /usr/bin/uname -m OUTPUT_VARIABLE VUO_HOST_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT CMAKE_OSX_ARCHITECTURES)
	set(CMAKE_OSX_ARCHITECTURES ${VUO_HOST_ARCH} CACHE STRING "" FORCE)
endif()
list(JOIN CMAKE_OSX_ARCHITECTURES " " buildArch)
message(STATUS "Building to run on: ${buildArch}")

include_directories(SYSTEM
	${CONAN_INCLUDE_DIRS_JSONC}
	${CONAN_INCLUDE_DIRS_LLVM}
	${CONAN_INCLUDE_DIRS_ZEROMQ}
)
include_directories(
	${PROJECT_BINARY_DIR}/base/config # For VuoConfig.h and coreTypes.h
)

set(CMAKE_C_COMPILER "${CONAN_BIN_DIRS_LLVM}/clang")
set(CMAKE_CXX_COMPILER "${CONAN_BIN_DIRS_LLVM}/clang++")
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
string(JOIN " " commonFlags
	-fcolor-diagnostics
	-fvisibility-inlines-hidden
	-g
	-Wdocumentation
	-Wno-documentation-deprecated-sync

	# Reduce file size.
	-fmerge-all-constants

	# Sync these warnings flags with VuoCompiler::readModuleFromC.
	-Wall
	-Wextra
	-Wimplicit-fallthrough
	-Wno-unused-parameter
	-Wno-c++11-extensions
	-Wno-sign-compare
	-Werror=implicit
)
string(JOIN " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}"
	-std=c99
	${commonFlags}
)
string(JOIN " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}"
	-stdlib=libc++
	-std=c++11
	${commonFlags}
)
string(JOIN " " CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS}"
	-std=c99
	${commonFlags}
)
string(JOIN " " CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS}"
	-stdlib=libc++
	-std=c++11
	${commonFlags}
)
set(CMAKE_C_FLAGS_RELEASE -Oz)
set(CMAKE_CXX_FLAGS_RELEASE -Oz)
set(CMAKE_OBJC_FLAGS_RELEASE -Oz)
set(CMAKE_OBJCXX_FLAGS_RELEASE -Oz)
set(CMAKE_BUILD_TYPE Release)

# Reduce file size.
# Explicitly specify the SDK version, since we still want some old behaviors.
set(CMAKE_SHARED_LINKER_FLAGS "-Xlinker -no_function_starts -Xlinker -dead_strip -Wl,-sdk_version,10.10")
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})

# If we modify a source file (e.g. VuoHeap.cc),
# re-link the dylib containing it (e.g. libVuoHeap.dylib),
# but don't re-link the dylibs/executables that link to that first dylib (e.g., vuo-compile).
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)


if (VUO_COMPILER_DEVELOPER)
	set(CMAKE_SKIP_RULE_DEPENDENCY ON)
endif()

if (VUO_PROFILE)
	add_compile_definitions(VUO_PROFILE)
endif()

enable_language(C)
enable_language(CXX)
enable_language(OBJC)
enable_language(OBJCXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

define_property(TARGET PROPERTY VuoGeneratedFiles
	BRIEF_DOCS "Files generated by this target"
	FULL_DOCS  "Files generated by this target"
)


include(VuoCompile)

if (EXISTS ${CMAKE_MODULE_PATH}/VuoCompilePro.cmake)
	include(VuoCompilePro)
	include(VuoPackage)
else()
	message(STATUS "Vuo Pro support is not available.")
endif()

include(VuoAnalyze)


# Adds files as resources to `target`'s macOS bundle in `Contents/subfolder`.
# This is only suitable for version-controlled files (not build products).
function (VuoBundleFiles target subfolder)
	set(files ${ARGN})

	target_sources(${target} PUBLIC ${files})
	set_property(SOURCE ${files} PROPERTY MACOSX_PACKAGE_LOCATION ${subfolder})
endfunction()

# Adds a linker command to create a binary section containing the specified Info.plist file.
function (VuoAddInfoPListSection target infoPlistFile)
	set(SHORT_VERSION "${VUO_VERSION_AND_BUILD}")
	configure_file(${infoPlistFile} Info.plist)
	target_link_options(${target} PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
endfunction()

# Provides macros so that source code can identify the architecture(s) for which it was built.
function (VuoDefineArchitectureMacros target)
	if ("x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES)
		target_compile_options(${target} PRIVATE -DVUO_TARGET_ARCHITECTURE_X86_64)
	endif()
	if ("arm64" IN_LIST CMAKE_OSX_ARCHITECTURES)
		target_compile_options(${target} PRIVATE -DVUO_TARGET_ARCHITECTURE_ARM64)
	endif()
endfunction()


# Qt =========================================================================
set(_qt5gui_OPENGL_INCLUDE_DIR "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/OpenGL.framework/Versions/A/Headers" CACHE STRING "" FORCE)
find_package(Qt5 REQUIRED COMPONENTS Widgets Test MacExtras Network PrintSupport Quick LinguistTools Svg)


# Precompiled prefix header ==================================================
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/empty.c)
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.c "")
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.cc "")
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.mm "")
endif()

# Builds a precompiled header.
function (VuoGeneratePCH target)
	cmake_parse_arguments(arg "" "TARGET" "LANGUAGES;DEFINITIONS" ${ARGV})

	add_library(${arg_TARGET} OBJECT)
	target_compile_definitions(${arg_TARGET} PRIVATE ${arg_DEFINITIONS})
	target_include_directories(${arg_TARGET}
		PRIVATE
			${PROJECT_SOURCE_DIR}/base
			${PROJECT_SOURCE_DIR}/compiler
			${PROJECT_SOURCE_DIR}/library
			${PROJECT_SOURCE_DIR}/type
			${PROJECT_BINARY_DIR}/base/config
			${PROJECT_BINARY_DIR}/type/list
			${PROJECT_BINARY_DIR}/lib/Vuo.framework/Headers
	)
	target_precompile_headers(${arg_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/vuo.pch)

	# Save time by only building each PCH for the languages that actually use it.
	if ("C" IN_LIST arg_LANGUAGES)
		target_sources(${arg_TARGET} PRIVATE empty.c)
	endif()
	if ("CXX" IN_LIST arg_LANGUAGES)
		target_sources(${arg_TARGET} PRIVATE empty.cc)
	endif()
	if ("OBJCXX" IN_LIST arg_LANGUAGES)
		target_sources(${arg_TARGET} PRIVATE empty.mm)
	endif()

	if ("VUO_PCH_QT" IN_LIST arg_DEFINITIONS OR "VUO_PCH_QTEST" IN_LIST arg_DEFINITIONS)
		target_link_libraries(${arg_TARGET} PRIVATE Qt5::Widgets)
	endif()

	if ("VUO_PCH_FRAMEWORK" IN_LIST arg_DEFINITIONS)
		target_compile_options(${arg_TARGET} PRIVATE "SHELL:-F ${PROJECT_BINARY_DIR}/lib")
		add_dependencies(${arg_TARGET} VuoFrameworkDylib)
	endif()

	# When the OSX_ARCHITECTURES target property is set (even to a single arch),
	# CMake outputs the `-Xarch_*` Clang argument, which `ccache` doesn't support.
	# If we're building for a single arch that's the same as the host arch,
	# unset OSX_ARCHITECTURES, enabling `ccache` to do its thing.
	if (CMAKE_OSX_ARCHITECTURES STREQUAL VUO_HOST_ARCH)
		set_property(TARGET ${arg_TARGET} PROPERTY OSX_ARCHITECTURES)
	endif()
endfunction()
VuoGeneratePCH(TARGET VuoPCHDefault              LANGUAGES C CXX OBJCXX    DEFINITIONS                                             )
VuoGeneratePCH(TARGET VuoPCHLLVM                 LANGUAGES   CXX OBJCXX    DEFINITIONS VUO_PCH_LLVM                                )
VuoGeneratePCH(TARGET VuoPCHLLVMQt               LANGUAGES   CXX OBJCXX    DEFINITIONS VUO_PCH_LLVM VUO_PCH_QT                     )
VuoGeneratePCH(TARGET VuoPCHQt                   LANGUAGES   CXX OBJCXX    DEFINITIONS              VUO_PCH_QT                     )
VuoGeneratePCH(TARGET VuoPCHQTestVuoFramework    LANGUAGES   CXX OBJCXX    DEFINITIONS              VUO_PCH_QTEST VUO_PCH_FRAMEWORK)

# Adds a precompiled (or prefix) header to the sources in the specified target.
# The second argument specifies which PCH to use (none for default, LLVM for VuoPCHLLVM, …).
function (VuoPCH target)
	set(pch VuoPCHDefault)
	if (ARGN)
		set(pch VuoPCH${ARGN})
	endif()

	target_precompile_headers(${target} REUSE_FROM ${pch})

	# Help Qt Creator parse the precompiled header correctly.
	get_target_property(defines ${pch} COMPILE_DEFINITIONS)
	if (defines)
		target_compile_definitions(${target} PRIVATE ${defines})
	endif()

	# Help Qt Creator find includes.
	get_target_property(includes ${pch} INCLUDE_DIRECTORIES)
	if (includs)
		target_include_directories(${target} PRIVATE ${includes})
	endif()

	# When the OSX_ARCHITECTURES target property is set (even to a single arch),
	# CMake outputs the `-Xarch_*` Clang argument, which `ccache` doesn't support.
	# If we're building for a single arch that's the same as the host arch,
	# unset OSX_ARCHITECTURES, enabling `ccache` to do its thing.
	if (CMAKE_OSX_ARCHITECTURES STREQUAL VUO_HOST_ARCH)
		set_property(TARGET ${target} PROPERTY OSX_ARCHITECTURES)
	endif()
endfunction()


# Symbol reduction ===========================================================
function (VuoGenerateUnexportedSymbolsList outputFile typeTargets)
	# Remove unused VuoType__moduleDetails symbols, to reduce file size.
	foreach (typeTarget ${typeTargets})
		get_target_property(typesInTarget "${typeTarget}" SOURCES)
		list(FILTER typesInTarget EXCLUDE REGEX "\\.o$")
		list(TRANSFORM typesInTarget REPLACE "\\.bc$" "__moduleDetails")
		list(TRANSFORM typesInTarget PREPEND "_")
		list(APPEND detailsSymbols "${typesInTarget}")
	endforeach()
	list(SORT detailsSymbols)
	list(REMOVE_DUPLICATES detailsSymbols)
	list(JOIN detailsSymbols "\n" detailsSymbolsString)
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/unexported_symbols_list.txt "${detailsSymbolsString}\n")
	set(${outputFile} "${CMAKE_CURRENT_BINARY_DIR}/unexported_symbols_list.txt" PARENT_SCOPE)
endfunction()


# Ad-hoc code-signing ========================================================
if (VUO_ADHOC_CODESIGN)
	# Add an ad-hoc code-signature, in order to be allowed to run on arm64.
	# Replace existing signature (if any).
	set(codesignAdHoc ${PROJECT_SOURCE_DIR}/base/codesignWrapper.sh --sign - --force)
	set(codesignAdHocVerify codesign --verify)
	set(codesignAdHocIgnoreReplace 2>&1 | (grep -F -v ': replacing existing signature' || true))
else()
	set(codesignAdHoc true)
	set(codesignAdHocVerify true)
	set(codesignAdHocIgnoreReplace "")
endif()


# Subprojects ================================================================
add_subdirectory(type)
add_subdirectory(base)
add_subdirectory(compiler)
add_subdirectory(runtime)
add_subdirectory(library)
add_subdirectory(node)
add_subdirectory(renderer)
add_subdirectory(framework)
add_subdirectory(translation)
add_subdirectory(editor)
add_subdirectory(test)
if (VUO_DOCUMENTATION)
	add_subdirectory(documentation)
endif()
if (EXISTS ${PROJECT_SOURCE_DIR}/package)
	add_subdirectory(package)
endif()


# Qt Creator =================================================================
set(ignore "${QT_QMAKE_EXECUTABLE}")


# In cmake-gui, hide unused/rarely-used variables ============================
mark_as_advanced(FORCE
	BUILD_TESTING
	CMAKE_BUILD_TYPE
	CMAKE_EXECUTABLE_FORMAT
	CONAN_FRAMEWORK_HapInAVFoundation_FOUND
	CONAN_FRAMEWORK_Syphon_FOUND
	CMAKE_INSTALL_PREFIX
	CMAKE_OSX_DEPLOYMENT_TARGET
	CMAKE_OSX_SYSROOT
	Qt5Core_DIR
	Qt5Gui_DIR
	Qt5LinguistTools_DIR
	Qt5MacExtras_DIR
	Qt5Network_DIR
	Qt5PrintSupport_DIR
	Qt5Qml_DIR
	Qt5Quick_DIR
	Qt5Svg_DIR
	Qt5Test_DIR
	Qt5Widgets_DIR
	Qt5_DIR
)
