file(GLOB examples
	RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/node/vuo.*/examples/*.vuo
)
if (CMAKE_SYSTEM_VERSION VERSION_LESS 17)
	# Don't test example compositions that require macOS 10.13 (Darwin 17.x.x) or later.
	list(REMOVE_ITEM examples
		../../node/vuo.image/examples/ShowWebPage.vuo
		../../node/vuo.image/examples/SpinSphereWithControlPanel.vuo
	)
endif()

list(REMOVE_ITEM examples
	# Skip self-terminating (`vuo.app.stopComposition`) compositions
	# since `runAndScreenshotExample.sh` expects them to run for at least 10 seconds.
	../../node/vuo.video/examples/SaveProcessedMovie.vuo

	# Skip example ocmpositions that block on user input.
	../../node/vuo.ui/examples/OpenImage.vuo
	../../node/vuo.ui/examples/SaveImage.vuo
)

VuoCompileCompositions(TestExamples ${examples})
add_dependencies(VuoTest TestExamples)

# See TestSDK for tests of /example/node and /example/runner.
# See TestNodes for tests of descriptions and deprecated nodes in example compositions.

# Creates a test that launches and screenshots each example composition.
foreach (example ${examples})
	get_filename_component(exampleDir ${example} DIRECTORY)
	get_filename_component(exampleDir ${exampleDir} ABSOLUTE)
	get_filename_component(exampleName ${example} NAME_WLE)
	set(testName TestExample_${exampleName})
	add_test(NAME ${testName}
		WORKING_DIRECTORY ${exampleDir}
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/runAndScreenshotExample.sh ${CMAKE_CURRENT_BINARY_DIR}/${exampleName}
	)
	set_tests_properties(${testName} PROPERTIES
		RESOURCE_LOCK GUI
		FIXTURES_REQUIRED "TestExamples;TestScreenshots"
	)
endforeach()

# Delete files created by these example compositions.
add_test(NAME TestExamplesCleanup
	COMMAND bash -c "rm -f ~/Desktop/WordInTheSpotlight.txt ~/Desktop/SepiaJellyfish.jpg ~/Desktop/ColdestTemperatures.csv ~/Desktop/CoinFlips.csv || true"
)
set_tests_properties(TestExamplesCleanup PROPERTIES
	FIXTURES_CLEANUP TestExamples
)
