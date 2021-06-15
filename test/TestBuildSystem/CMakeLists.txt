file(GLOB features features/*.feature)
set(priorTest "")
foreach (feature ${features})
	get_filename_component(featureName "${feature}" NAME_WLE)
	set(testName "TestBuildSystem-${featureName}")
	add_test(NAME "${testName}"
		COMMAND behat --stop-on-failure --colors --format=pretty --out=std ${feature}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
	set_tests_properties("${testName}" PROPERTIES
		ENVIRONMENT BEHAT_PARAMS={"suites":{"default":{"contexts":[{"FeatureContext":{"sourceDir":"${PROJECT_SOURCE_DIR}","testDir":"${CMAKE_CURRENT_BINARY_DIR}"}}]}}}
		RUN_SERIAL ON
		TIMEOUT 14400

		# Make the tests run in lexicographic order
		# (the `0-parallel` test cleans and builds everything,
		# so we might as well run it first to build everything for the later tests).
		DEPENDS "${priorTest}"
	)
	set(priorTest "${testName}")
endforeach()
