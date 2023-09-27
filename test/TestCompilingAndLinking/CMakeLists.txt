VuoTest(NAME TestCompilingAndLinking
	SOURCE TestCompilingAndLinking.cc
	FIXTURE TestCompilingAndLinking

	# This test modifies shared caches, which can trip up other tests.
	RUN_SERIAL ON
)
target_link_libraries(TestCompilingAndLinking
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestCompilingAndLinking/vuo.test.triggerCarryingFloat.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingInteger.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingPoint3d.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingPoint4d.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingReal.c
)
VuoCompileTypes(
	node-TestCompilingAndLinking/VuoTestFloat.c
)
target_sources(TestCompilingAndLinking PRIVATE
	node-TestCompilingAndLinking/VuoTestFloat.h
)
add_dependencies(TestCompilingAndLinking
	VuoEditorApp
	TestCompilingAndLinking.nodes
	TestCompilingAndLinking.types
)

add_library(UserDylib SHARED
	EXCLUDE_FROM_ALL
	node-TestCompilingAndLinking/userDylib.c
)
set_target_properties(UserDylib PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/test/TestCompilingAndLinking
)
add_dependencies(TestCompilingAndLinking
	UserDylib
)
add_custom_command(TARGET UserDylib POST_BUILD COMMAND ${codesignAdHoc} libUserDylib.dylib)

# Delete modules installed by this test.
add_test(NAME TestCompilingAndLinkingCleanup
	COMMAND rm -f
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.dependsOnUserDylib.c
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/libUserDylib.dylib
)
set_tests_properties(TestCompilingAndLinkingCleanup PROPERTIES
	FIXTURES_CLEANUP TestCompilingAndLinking
)
