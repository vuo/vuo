VuoTest(NAME TestModuleLoading
	SOURCE TestModuleLoading.cc
	RESOURCE_LOCK COMPILER
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
add_dependencies(TestModuleLoading TestModuleLoading.nodes)
