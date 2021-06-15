VuoTest(NAME TestModuleLoading
	SOURCE TestModuleLoading.cc

	# This test installs modules to shared folders, which can trip up other tests.
	RUN_SERIAL ON
	FIXTURE TestModuleLoading
)
target_link_libraries(TestModuleLoading
	PRIVATE
	VuoRenderer
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestModuleLoading/vuo.test.TestModuleLoading.control.c
	node-TestModuleLoading/vuo.test.TestModuleLoading.nodeClass.c
	node-TestModuleLoading/vuo.test.TestModuleLoading.dependsOnNodeClass.c
	node-TestModuleLoading/vuo.test.TestModuleLoading.dependsOnSubcomposition.c
)
add_dependencies(TestModuleLoading
	VuoEditorApp
	TestModuleLoading.nodes
)

# Delete modules installed by this test.
add_test(NAME TestModuleLoadingCleanup
	COMMAND rm -f
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.TestModuleLoading.control.vuonode
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.TestModuleLoading.dependsOnNodeClass.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.TestModuleLoading.dependsOnNodeClass.vuonode
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.TestModuleLoading.dependsOnSubcomposition.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.TestModuleLoading.dependsOnSubcomposition.vuonode
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.TestModuleLoading.nodeClass.vuonode
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.TestModuleLoading.override.vuo
		$ENV{HOME}/Library/Application\ Support/Vuo/Modules/vuo.test.TestModuleLoading.subcomposition.vuo
)
set_tests_properties(TestModuleLoadingCleanup PROPERTIES
	FIXTURES_CLEANUP TestModuleLoading
)
