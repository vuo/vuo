add_custom_target(VuoTest)

# Adds steps to build and run a test.
function (VuoTest)
	cmake_parse_arguments(arg "" "NAME;SOURCE;COST;TIMEOUT;FIXTURE" "RESOURCE_LOCK" ${ARGV})

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

	if (VUO_TEST_OUTPUT_XML)
		set(testXML "-xml -o ${PROJECT_BINARY_DIR}/test/${arg_NAME}.qtest")
	endif()
	if (NOT arg_NAME STREQUAL TestVuoVideo)
		set(minimumValue "-minimumvalue 1000")
	endif()

	add_test(NAME ${arg_NAME}
		# Allow setting an execution-time environment variable to add arguments to the test invocation
		# (can't use $ARG{} since that requires re-running CMake).  For example:
		#     ARG="testAddingAndRemovingModules:compiled node class, user, not loading all modules" ctest -V -R ModuleLoading
		COMMAND bash -c "ARG=\"$(printenv ARG)\" ;
			if [ -n \"\$ARG\" ]; then
				${PROJECT_BINARY_DIR}/bin/${arg_NAME} -nocrashhandler ${minimumValue} -maxwarnings 10000 ${testXML} \"\$ARG\"
			else
				${PROJECT_BINARY_DIR}/bin/${arg_NAME} -nocrashhandler ${minimumValue} -maxwarnings 10000 ${testXML}
			fi
			"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)

	if (arg_RESOURCE_LOCK)
		set_tests_properties(${arg_NAME} PROPERTIES
			RESOURCE_LOCK "${arg_RESOURCE_LOCK}"
		)
	endif()
	if (arg_COST)
		set_tests_properties(${arg_NAME} PROPERTIES
			COST ${arg_COST}
		)
	endif()
	if (arg_TIMEOUT)
		set_tests_properties(${arg_NAME} PROPERTIES
			TIMEOUT ${arg_TIMEOUT}
		)
	endif()
	if (arg_FIXTURE)
		set_tests_properties(${arg_NAME} PROPERTIES
			FIXTURES_REQUIRED ${arg_FIXTURE}
		)
	endif()
	if (EXISTS /Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib)
		set_tests_properties(${arg_NAME} PROPERTIES
			ENVIRONMENT DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/usr/lib/libMainThreadChecker.dylib
			FAIL_REGULAR_EXPRESSION "Main Thread Checker: UI API called on a background thread"
		)
	endif()
endfunction()


# Listed in approximately low-level (unit) to high-level (integration/end-to-end) order.

add_subdirectory(TestVuoUtilities)
add_subdirectory(TestVuoDirectedAcyclicGraph)
add_subdirectory(TestVuoProtocol)
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
add_subdirectory(TestDocumentation)

# All of the above tests should complete in less than a minute.
# The tests below take significantly longer than that, and/or interfere with the GUI.
if (VUO_TEST_LONG)
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
	add_subdirectory(TestBuiltProducts)
	add_subdirectory(TestExamples)
	add_subdirectory(TestSDK)

	# GUI.
	add_subdirectory(TestProtocolDrivers)
	add_subdirectory(TestWindow)
	add_subdirectory(TestVuoEditor)
	add_subdirectory(TestInputEditors)
	add_subdirectory(TestCompositions)
	add_subdirectory(TestNodes)
endif()

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
