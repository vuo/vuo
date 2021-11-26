add_custom_target(VuoTest)

option(VUO_TEST_WITH_GMALLOC  "Whether to run tests using libgmalloc, to help detect buffer overruns and use-after-free errors." OFF)

include(QtTest)
configure_file("CTestCustom.cmake.in" "${PROJECT_BINARY_DIR}/CTestCustom.cmake")

# Adds steps to build and run a test.
#
# If the GMALLOC_ELIGIBLE option is specified, the entire test executable will run with libgmalloc when the VUO_TEST_WITH_GMALLOC option is also enabled.
# Tests that use LLVM/VuoCompiler shouldn't specify GMALLOC_ELIGIBLE, since LLVM makes a huge number of tiny allocations, overwhelming the macOS kernel.
# Instead, those tests should call `VuoRunner::setRuntimeChecking(true)`, to run just the composition (not the compiler) with libgmalloc.
function (VuoTest)
	cmake_parse_arguments(arg "GMALLOC_ELIGIBLE" "NAME;SOURCE;TIMEOUT;FIXTURE;RUN_SERIAL" "" ${ARGV})

	add_executable(${arg_NAME} EXCLUDE_FROM_ALL ${arg_SOURCE})
	set(rpaths
		@loader_path/../bin/Vuo.app/Contents/Frameworks
		@loader_path/../bin/Vuo.app/Contents/Frameworks/Vuo.framework/Versions/Current/Modules
		${CONAN_LIB_DIRS_WJELEMENT}
		# For QTestLib.
		${CONAN_LIB_DIRS_QT}
	)
	set_target_properties(${arg_NAME} PROPERTIES
		AUTOMOC ON

		# Instead of using the default rpaths…
		BUILD_WITH_INSTALL_RPATH TRUE
		# Use just the rpaths from Vuo Editor (plus those needed only for tests).
		INSTALL_RPATH "${rpaths}"
	)
	target_compile_definitions(${arg_NAME}
		PRIVATE
		BINARY_DIR="${PROJECT_BINARY_DIR}"
	)
	target_include_directories(${arg_NAME}
		PRIVATE
		../../node/vuo.artnet
		../../node/vuo.data
		../../node/vuo.image
		../../node/vuo.layer
		../../node/vuo.math
		../../node/vuo.midi
		../../node/vuo.scene
		../../node/vuo.table
		../../node/vuo.time
		../../node/vuo.ui
	)
	target_link_libraries(${arg_NAME}
		PUBLIC
		# Link to Vuo.framework first, so OS X 10.10's linker prefers our libxml2.dylib over the system's.
		VuoFrameworkDylib

		"-framework CoreFoundation"
		"-framework OpenGL"
		Qt5::Test
		CONAN_PKG::curl
		CONAN_PKG::freeimage
		CONAN_PKG::muparser
		CONAN_PKG::oai
		vuo.core.libraries
	)
	if (arg_NAME STREQUAL TestBuiltProducts
	 OR arg_NAME STREQUAL TestConsole
	 OR arg_NAME STREQUAL TestEditorCommands
	 OR arg_NAME STREQUAL TestExport
	 OR arg_NAME STREQUAL TestInputEditors
	 OR arg_NAME STREQUAL TestNodes
	 OR arg_NAME STREQUAL TestVuoEditor
	 OR arg_NAME STREQUAL TestVuoRenderer)
		VuoPCH(${arg_NAME} Qt)
	else()
		VuoPCH(${arg_NAME} QTestVuoFramework)
	endif()
	add_dependencies(${arg_NAME} VuoFramework)

	add_dependencies(VuoTest ${arg_NAME})

	if (arg_RUN_SERIAL)
		list(APPEND testProperties RUN_SERIAL ${arg_RUN_SERIAL})
	endif()

	if (arg_TIMEOUT)
		list(APPEND testProperties TIMEOUT ${arg_TIMEOUT})
	endif()

	if (arg_FIXTURE)
		list(APPEND testProperties FIXTURES_REQUIRED ${arg_FIXTURE})
	endif()

	if (VUO_TEST_WITH_GMALLOC)
		list(APPEND testEnvironment VUO_RUNTIME_CHECKING=/usr/lib/libgmalloc.dylib)
	endif()

	if (GMALLOC_ELIGIBLE AND VUO_TEST_WITH_GMALLOC)
		list(APPEND testEnvironment DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib)
	elseif (EXISTS /Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib)
		list(APPEND testEnvironment DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib)
		list(APPEND testFailRegexes "Main Thread Checker: UI API called on a background thread")
	endif()

	if (NOT arg_NAME STREQUAL TestReferenceCounting)
		list(APPEND testFailRegexes "(VuoRegister|VuoRegisterSingleton|VuoRetain|VuoRelease) was (not called enough times|called (more than once|for (bogus|unregistered|unretained) pointer))")
	endif()

	list(APPEND testFailRegexes "Function has no testdata|Unknown test function|Unknown testdata for function")

	list(JOIN testFailRegexes "|" testFailRegexes)
	list(APPEND testProperties FAIL_REGULAR_EXPRESSION "\"${testFailRegexes}\"")

	qtest_discover_tests(${arg_NAME}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DISCOVERY_TIMEOUT 600
		PROPERTIES ${testProperties}
		ENVIRONMENT ${testEnvironment}
	)
endfunction()


add_subdirectory(TestTests)

# Listed in approximately low-level (unit) to high-level (integration/end-to-end) order.

add_subdirectory(TestVuoUtilities)
add_subdirectory(TestVuoDirectedAcyclicGraph)
add_subdirectory(TestVuoProtocol)
add_subdirectory(TestVuoCompilerCompatibility)
add_subdirectory(TestVuoRunnerCocoa)

add_subdirectory(TestVuoCompiler)
	add_subdirectory(TestVuoCompilerBitcodeGenerator)
	add_subdirectory(TestVuoCompilerModule)
	add_subdirectory(TestVuoCompilerType)
	add_subdirectory(TestVuoCompilerNodeClass)
	add_subdirectory(TestVuoCompilerNode)
	add_subdirectory(TestVuoCompilerComposition)
	add_subdirectory(TestVuoCompilerGraphvizParser)

add_subdirectory(TestVuoRenderer)
add_subdirectory(TestVuoTypes)
add_subdirectory(TestShaderFile)

add_subdirectory(TestCompositionExecution)
	add_subdirectory(TestVuoIsfModuleCompiler)
	add_subdirectory(TestImageFilters)
	add_subdirectory(TestNodeExecutionOrder)
	add_subdirectory(TestReferenceCounting)

add_subdirectory(TestHeap)
add_subdirectory(TestModules)
if (VUO_EXAMPLES)
	add_subdirectory(TestExamples)
endif()
add_subdirectory(TestDocumentation)

# Long.
add_subdirectory(TestCompilingAndLinking)
add_subdirectory(TestModuleLoading)
add_subdirectory(TestControlAndTelemetry)
add_subdirectory(TestSubcompositions)
add_subdirectory(TestEventDropping)
add_subdirectory(TestVuoRunner)
add_subdirectory(TestTypes)
add_subdirectory(TestVuoVideo)
add_subdirectory(TestBuildSystem)
add_subdirectory(TestSDK)

# GUI.
add_subdirectory(TestProtocolDrivers)
add_subdirectory(TestWindow)
add_subdirectory(TestEditorCommands)
add_subdirectory(TestInputEditors)
add_subdirectory(TestConsole)
add_subdirectory(TestCompositions)
add_subdirectory(TestNodes)

if (VuoPro)
	include(pro/pro.cmake)
endif()


# Generate a screenshot gallery after all the screenshots have been taken.
add_test(NAME TestScreenshotGallery
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generateScreenshotGallery.sh
)
set_tests_properties(TestScreenshotGallery PROPERTIES
	FIXTURES_CLEANUP TestScreenshots
)
