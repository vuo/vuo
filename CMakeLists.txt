cmake_minimum_required(VERSION 3.15...3.17 FATAL_ERROR)


# Configuration ==============================================================
project(Vuo VERSION 2.0.3 LANGUAGES NONE)

# The major-version component of ${Vuo_VERSION}.
# Used in framework paths (e.g., `Vuo.framework/Versions/2/Vuo`).
set(VuoFrameworkVersion 2)

option(VUO_COMPILER_DEVELOPER "Compiler development mode.  Allows you to make changes to VuoBase and VuoCompiler without rebuilding all nodes and types (but you're responsible for keeping track of whether nodes and types need to be rebuilt).  Saves several minutes per edit-compile-test cycle." OFF)
option(VUO_NODE_DEVELOPER     "Node development mode.  Allows you to make changes to nodes, types, and libraries without rebuilding the Vuo.framework cache.  Saves about 30 seconds per edit-compile-test cycle (but compositions take longer to start up initially)." OFF)
option(VUO_HEAP_TRACE         "Logs reference-counting errors to stderr (in addition to the usual telemetry-based error reporting), including backtraces.  Also enables using VuoHeap_addTrace().")
option(VUO_HEAP_TRACE_ALL     "Logs all reference-counting operations.  Very noisy.")
option(VUO_TEST_LONG          "Run all tests (including those that take a long time)." OFF)
option(VUO_TEST_OUTPUT_XML    "Output test results as XML files." OFF)
option(VUO_PROFILE            "Add instrumentation for performance profiling." OFF)
option(VUO_ANALYZE            "Run Clang Analyzer and other time-consuming analysis tools." OFF)
option(VUO_PRECOMPILE_HEADERS "Precompile common headers to improve build performance." OFF)


# Commands ===================================================================
if (EXISTS /Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib)
   set(mainThreadChecker DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib)
endif()
add_custom_target(go COMMAND ${CMAKE_COMMAND} -E env ${mainThreadChecker} ${PROJECT_BINARY_DIR}/bin/Vuo.app/Contents/MacOS/Vuo USES_TERMINAL)


# Early checks ===============================================================
if (PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "\nOnly build Vuo outside the source tree.  For example:\n    rm -Rf CMakeCache.txt CMakeFiles\n    mkdir build\n    cd build\n    cmake ..")
endif()


# Version/revision ===========================================================
set(buildId "$ENV{BUILD_ID}")
if (NOT buildId)
	get_filename_component(DIRNAME "${PROJECT_SOURCE_DIR}" NAME)
	set(VUO_VERSION_AND_BUILD "${Vuo_VERSION}.${DIRNAME}")
else()
	set(VUO_VERSION_AND_BUILD "${Vuo_VERSION}.${buildId}")
endif()

set(VUO_REVISION "$ENV{SVN_REVISION}")
if (NOT VUO_REVISION)
	execute_process(COMMAND /usr/local/bin/svnversion -n ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE VUO_REVISION)
endif()
if (buildId)
	set(VUO_REVISION "${VUO_REVISION}, build ${buildId}")
endif()


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
set(CMAKE_C_COMPILER_VERSION 3.3)
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
	LLVMMCDisassembler
	LLVMMCJIT
	LLVMOption
	LLVMTableGen
	LTO
	clang
	clangASTMatchers
	clangFormat
	clangTooling
)

# Exclude LLVM/Clang libraries distributed with macOS.
list(REMOVE_ITEM CONAN_LIBS_LLVM
	c++
	c++abi
)


# Compilers ==================================================================
enable_testing()

set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
if (NOT EXISTS ${CMAKE_OSX_SYSROOT})
	message(FATAL_ERROR "\nPlease install ${CMAKE_OSX_SYSROOT}")
endif()

include_directories(SYSTEM
	${CONAN_INCLUDE_DIRS_JSONC}
	${CONAN_INCLUDE_DIRS_LLVM}
	${CONAN_INCLUDE_DIRS_ZEROMQ}
)
include_directories(
	${PROJECT_BINARY_DIR}/base/config # For VuoConfig.h and coreTypes.h
)

set(CMAKE_C_COMPILER "${CONAN_BIN_DIRS_LLVM}/clang")
set(CMAKE_C_SOURCE_FILE_EXTENSIONS c;m)
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_COMPILER "${CONAN_BIN_DIRS_LLVM}/clang++")
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
string(JOIN " " commonFlags
	-march=x86-64
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
set(CMAKE_C_FLAGS_RELEASE -Oz)
set(CMAKE_CXX_FLAGS_RELEASE -Oz)
set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS cc;cpp;mm)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_BUILD_TYPE Release)

# Reduce file size.
set(CMAKE_SHARED_LINKER_FLAGS "-Xlinker -no_function_starts -Xlinker -dead_strip")
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})


if (VUO_COMPILER_DEVELOPER)
	set(CMAKE_SKIP_RULE_DEPENDENCY ON)
endif()

if (VUO_PROFILE)
	add_compile_definitions(VUO_PROFILE)
endif()

enable_language(C)
enable_language(CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

define_property(TARGET PROPERTY VuoGeneratedFiles
	BRIEF_DOCS "Files generated by this target"
	FULL_DOCS  "Files generated by this target"
)
define_property(TARGET PROPERTY VuoPCHUseCount
	BRIEF_DOCS "For PCH targets (see VuoGeneratePCH), the number of source files that use this PCH"
	FULL_DOCS  "For PCH targets (see VuoGeneratePCH), the number of source files that use this PCH"
)


include(VuoCompile)

if (EXISTS ${CMAKE_MODULE_PATH}/VuoCompilePro.cmake)
	include(VuoCompilePro)
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


# Qt =========================================================================
find_package(Qt5 REQUIRED COMPONENTS Widgets Test MacExtras Network PrintSupport Quick LinguistTools Svg)


# Prefix header ==============================================================
set(VuoPCHDir ${PROJECT_BINARY_DIR}/pch)
file(MAKE_DIRECTORY ${VuoPCHDir})
set_property(GLOBAL PROPERTY VuoPCH)
# Builds a precompiled header.
function (VuoGeneratePCH target)
	set(pch ${VuoPCHDir}/${target}.pch)

	get_directory_property(defines COMPILE_DEFINITIONS)
	string(JOIN ";-D" defines ${ARGN} ${defines})
	if (defines)
		set(defines "-D${defines}")
	endif()

	get_directory_property(includes INCLUDE_DIRECTORIES)
	string(JOIN ";-isystem;" includes ${includes})

	set(cxxFlags "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
	separate_arguments(cxxFlags)

	if ("VUO_PCH_FRAMEWORK" IN_LIST ARGN)
		set(extraDepends
			${PROJECT_BINARY_DIR}/lib/Vuo.framework/Versions/${VuoFrameworkVersion}/Headers/Vuo.h
			${PROJECT_BINARY_DIR}/lib/Vuo.framework/Versions/${VuoFrameworkVersion}/Headers/type.h
		)
	endif()

	add_custom_command(
		DEPENDS
			${PROJECT_SOURCE_DIR}/vuo.pch
			${PROJECT_SOURCE_DIR}/library/VuoLog.h
			${extraDepends}
		IMPLICIT_DEPENDS CXX ${PROJECT_SOURCE_DIR}/vuo.pch
		COMMENT "Precompiling ${target}"
		COMMAND ${CMAKE_CXX_COMPILER}
			${defines}
			-isystem ${includes}
			-iframework ${CONAN_QT_ROOT}/lib
			-F ${PROJECT_BINARY_DIR}/lib
			-I base/config
			-I ../base
			-I ../compiler
			-I ../library
			${cxxFlags}
			-isysroot ${CMAKE_OSX_SYSROOT}
			-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}
			-fPIC
			-x c++-header
			${PROJECT_SOURCE_DIR}/vuo.pch
			-o ${pch}
		OUTPUT ${pch}
	)
	add_custom_target(${target} DEPENDS ${pch})
	set_target_properties(${target} PROPERTIES
		COMPILE_DEFINITIONS "${ARGN}"
		VuoPCHUseCount 0
	)

	if ("VUO_PCH_FRAMEWORK" IN_LIST ARGN)
		add_dependencies(${target} VuoFrameworkDylib)
	endif()

	get_property(VuoPCH GLOBAL PROPERTY VuoPCH)
	list(APPEND VuoPCH ${target})
	set_property(GLOBAL PROPERTY VuoPCH "${VuoPCH}")
endfunction()
VuoGeneratePCH(VuoPCHDefault                                                       )
VuoGeneratePCH(VuoPCHLLVM              VUO_PCH_LLVM                                )
VuoGeneratePCH(VuoPCHLLVMQt            VUO_PCH_LLVM VUO_PCH_QT                     )
VuoGeneratePCH(VuoPCHQt                             VUO_PCH_QT                     )
VuoGeneratePCH(VuoPCHQTestVuoFramework              VUO_PCH_QTEST VUO_PCH_FRAMEWORK)

# Adds a precompiled (or prefix) header to the sources in the specified target.
# The second argument specifies which PCH to use (none for default, LLVM for VuoPCHLLVM, …).
function (VuoPCH target)
	set(pch VuoPCHDefault)
	if (ARGN)
		set(pch VuoPCH${ARGN})
	endif()

	if (VUO_PRECOMPILE_HEADERS)

		add_dependencies(${target} ${pch})

		# Use the PCH for C++ sources.
		get_target_property(cxxSources ${target} SOURCES)
		list(FILTER cxxSources INCLUDE REGEX "\.cc$$")

		get_target_property(automoc ${target} AUTOMOC)
		if (automoc)
			list(APPEND cxxSources ${target}_autogen/mocs_compilation.cpp)
		endif()
		get_target_property(defines ${pch} COMPILE_DEFINITIONS)
		foreach (source ${cxxSources})
			set_property(SOURCE ${source} APPEND PROPERTY COMPILE_FLAGS "-include-pch ${VuoPCHDir}/${pch}.pch")
			set_property(SOURCE ${source} APPEND PROPERTY COMPILE_DEFINITIONS "${defines}")
		endforeach()

		# Merely prefix the header for non-C++ sources.
		get_target_property(nonCXXSources ${target} SOURCES)
		list(FILTER nonCXXSources EXCLUDE REGEX "\.cc$$")
		foreach (source ${nonCXXSources})
			set_property(SOURCE ${source} APPEND PROPERTY COMPILE_FLAGS "-include ${PROJECT_SOURCE_DIR}/vuo.pch")
			set_property(SOURCE ${source} APPEND PROPERTY COMPILE_DEFINITIONS "${defines}")
		endforeach()

		# Tell Qt Creator that we're using a PCH.
		# https://bugreports.qt.io/browse/QTCREATORBUG-20590
		set(QTC_${target}_PCH ${PROJECT_SOURCE_DIR}/vuo.pch CACHE INTERNAL "")

		if (VUO_PROFILE)
			# Keep stats.
			get_target_property(pchUseCount ${pch} VuoPCHUseCount)
			list(LENGTH cxxSources sourceCount)
			math(EXPR pchUseCount "${pchUseCount} + ${sourceCount}")
			set_target_properties(${pch} PROPERTIES VuoPCHUseCount ${pchUseCount})
		endif()

	else()

		# Merely prefix the header.
		get_target_property(sources ${target} SOURCES)

		get_target_property(automoc ${target} AUTOMOC)
		if (automoc)
			list(APPEND sources ${target}_autogen/mocs_compilation.cpp)
		endif()

		set_property(SOURCE ${sources} APPEND PROPERTY COMPILE_FLAGS "-iframework ${CONAN_QT_ROOT}/lib -include ${PROJECT_SOURCE_DIR}/vuo.pch")

		get_target_property(defines ${pch} COMPILE_DEFINITIONS)
		set_property(SOURCE ${sources} APPEND PROPERTY COMPILE_DEFINITIONS "${defines}")

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
add_subdirectory(documentation)
if (EXISTS ${PROJECT_SOURCE_DIR}/package)
	add_subdirectory(package)
endif()


# Qt Creator =================================================================
set(ignore "${QT_QMAKE_EXECUTABLE}")


# In cmake-gui, hide unused/rarely-used variables ============================
mark_as_advanced(FORCE
	CMAKE_BUILD_TYPE
	CMAKE_EXECUTABLE_FORMAT
	CMAKE_INSTALL_PREFIX
	CMAKE_OSX_ARCHITECTURES
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


# Info about build performance ===============================================
if (VUO_PROFILE)
	get_property(VuoPCH GLOBAL PROPERTY VuoPCH)
	foreach (pch ${VuoPCH})
		set(pchPadded "${pch}                       ")
		string(SUBSTRING ${pchPadded} 0 23 pchPadded)
		get_target_property(pchUseCount ${pch} VuoPCHUseCount)
		message(STATUS "${pchPadded} used by ${pchUseCount}")
	endforeach()
endif()
