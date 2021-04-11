VuoTest(NAME TestCompilingAndLinking
	SOURCE TestCompilingAndLinking.cc

	# This test modifies shared caches, which can trip up other tests.
	RUN_SERIAL ON
)
target_link_libraries(TestCompilingAndLinking
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestCompilingAndLinking/vuo.test.doNothing.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingFloat.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingInteger.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingPoint3d.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingPoint4d.c
	node-TestCompilingAndLinking/vuo.test.triggerCarryingReal.c
)
VuoCompileTypes(
	node-TestCompilingAndLinking/VuoTestFloat.c
)
add_dependencies(TestCompilingAndLinking
	VuoEditorApp
	TestCompilingAndLinking.nodes
	TestCompilingAndLinking.types
)
