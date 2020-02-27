if (VUO_TEST_OUTPUT_XML)
	set(testXML --format junit --out ${PROJECT_BINARY_DIR}/test/TestBuildSystem.junit)
endif()
add_test(NAME TestBuildSystem
	COMMAND behat --colors --format=pretty --out=std ${testXML}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(TestBuildSystem PROPERTIES
	ENVIRONMENT BEHAT_PARAMS={"suites":{"default":{"contexts":[{"FeatureContext":{"sourceDir":"${PROJECT_SOURCE_DIR}","testDir":"${CMAKE_CURRENT_BINARY_DIR}"}}]}}}
	RUN_SERIAL ON
)
