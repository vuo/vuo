if (NOT CMAKE_SCRIPT_MODE_FILE)

	# Create a test that runs this CMakeLists.txt file as a script (below).
	add_test(NAME TestSDK
		COMMAND ${CMAKE_COMMAND}
			-DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
			-DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
			-DCONAN_QT_ROOT=${CONAN_QT_ROOT}
			-DVUO_VERSION_AND_BUILD=${VUO_VERSION_AND_BUILD}
			-DVUO_TEST_WITH_GMALLOC=${VUO_TEST_WITH_GMALLOC}
			-DVuoFrameworkVersion=${VuoFrameworkVersion}
			-DVuoPro=${VuoPro}
			-P ${CMAKE_CURRENT_LIST_FILE}
	)
	set_tests_properties(TestSDK PROPERTIES
		FIXTURES_REQUIRED TestSDK
	)

	add_test(NAME TestSDKCleanup
		COMMAND rm -f
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/example.customtype.vuonode
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/example.imageFilter.brightness.vuonode
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/example.imageFilter.coreImage.vuonode
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/example.imageFilter.metal.vuonode
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/example.stateful.countExecutions.vuonode
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/example.stateless.vuoize.vuonode
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/libExampleImageFilterCoreImage.a
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/libExampleImageFilterMetal.a
			$ENV{HOME}/Library/Application\ Support/Vuo/Modules/libExampleLanguageInputEditor.dylib
	)
	set_tests_properties(TestSDKCleanup PROPERTIES
		FIXTURES_CLEANUP TestSDK
	)

else()

	function(execute)
		cmake_parse_arguments(arg "TIMEOUT;GMALLOC_ELIGIBLE" "" "COMMAND" ${ARGV})
		if (arg_TIMEOUT)
			set(timeout TIMEOUT 6)
		endif()

		if(GMALLOC_ELIGIBLE AND VUO_TEST_WITH_GMALLOC)
			message(WARNING "using libgmalloc")
			set(ENV{DYLD_INSERT_LIBRARIES} /usr/lib/libgmalloc.dylib)
		endif()
		execute_process(WORKING_DIRECTORY ${buildDir} COMMAND ${arg_COMMAND} ${timeout} RESULT_VARIABLE ret)
		unset(ENV{DYLD_INSERT_LIBRARIES})
		if (ret STREQUAL "Process terminated due to timeout")
			message("Stopping normally after timeout")
		elseif (ret)
			message(FATAL_ERROR "${arg_COMMAND} failed")
		endif()
	endfunction()

	set(sdkDir ${PROJECT_BINARY_DIR}/package/vuo-${VUO_VERSION_AND_BUILD}--sdk)

	# Ensure that all the node and VuoRunner examples build and run.
	foreach (example
		node/customType
		node/imageFilterGLSL
		node/imageFilterCoreImage
		node/imageFilterMetal
		node/stateful
		node/stateless
		runner/CompileAndRunInCurrentProcess
		runner/CompileAndRunInNewProcess
		runner/DumpTelemetryMessages
		runner/ListPublishedPorts
		runner/RunImageFilter-GLFW
		runner/RunImageFilter-Qt
	)
		set(buildDir ${sdkDir}/example/${example}/build)
		file(MAKE_DIRECTORY ${buildDir})
		execute(COMMAND cmake .. -DCMAKE_FRAMEWORK_PATH=../../../framework -DCMAKE_PREFIX_PATH=${CONAN_QT_ROOT}/lib/cmake)
		execute(COMMAND make -j9)
		if (example STREQUAL "runner/RunImageFilter-GLFW")
			execute(TIMEOUT GMALLOC_ELIGIBLE COMMAND ./RunImageFilter-current-makeFromJson               )
			execute(TIMEOUT GMALLOC_ELIGIBLE COMMAND ./RunImageFilter-current-makeFromJsonWithDimensions )
			execute(TIMEOUT GMALLOC_ELIGIBLE COMMAND ./RunImageFilter-separate-makeFromJson              )
			execute(TIMEOUT GMALLOC_ELIGIBLE COMMAND ./RunImageFilter-separate-makeFromJsonWithDimensions)
			execute(TIMEOUT GMALLOC_ELIGIBLE COMMAND ./RunImageFilter-separate-interprocessToFramebuffer )
			execute(TIMEOUT GMALLOC_ELIGIBLE COMMAND ./RunImageFilter-separate-interprocessToTexture     )
		elseif (example MATCHES "^runner/")
			file(COPY ${PROJECT_BINARY_DIR}/bin/qt.conf DESTINATION ${buildDir})
			get_filename_component(exampleBinary ${example} NAME)
			execute(TIMEOUT GMALLOC_ELIGIBLE COMMAND ./${exampleBinary})
		endif()
	endforeach()

	set(buildDir ${sdkDir}/example/runner/VuoPluginApp)
	# Normally VuoPluginApp uses an absolute path to the system-wide Vuo.framework installed by the PKG.
	# For this test, use a relative path to the Vuo.framework we just built (without needing to install it system-wide).
	set(xcodeProjectFile ${buildDir}/VuoPluginApp.xcodeproj/project.pbxproj)
	message("xcodeProjectFile=${xcodeProjectFile}")
	file(READ ${xcodeProjectFile} xcodeProjectContents)
	string(REPLACE "path = /Library/Developer/Vuo/framework/Vuo.framework; sourceTree = \"<absolute>\";" "path = ../../../framework/Vuo.framework; sourceTree = SOURCE_ROOT;" xcodeProjectContents "${xcodeProjectContents}")
	string(REPLACE "/Library/Developer/Vuo/framework" "../../../framework" xcodeProjectContents "${xcodeProjectContents}")
	file(WRITE ${xcodeProjectFile} "${xcodeProjectContents}")
	execute(COMMAND xcodebuild)
	execute(TIMEOUT GMALLOC_ELIGIBLE COMMAND build/Release/VuoPluginApp.app/Contents/MacOS/VuoPluginApp)

	if (VuoPro)
		include(${PROJECT_SOURCE_DIR}/test/TestSDK/pro/pro.cmake)
	endif()

endif()
