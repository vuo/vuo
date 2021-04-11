VuoTest(NAME TestModuleLoading
	SOURCE TestModuleLoading.cc

	# This test installs modules to shared folders, which can trip up other tests.
	RUN_SERIAL ON
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
