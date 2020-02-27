VuoTest(NAME TestCompositions
	SOURCE TestCompositions.cc
	RESOURCE_LOCK GUI;COMPILER
)
target_sources(TestCompositions
	PRIVATE
	PortConfiguration.cc
)
target_link_libraries(TestCompositions
	PRIVATE
	TestCompositionExecution
)

VuoCompileNodes(
	node-TestCompositions/vuo.test.delay.c
	node-TestCompositions/vuo.test.mesh.c
	node-TestCompositions/vuo.test.wallTime.c
)
add_dependencies(TestCompositions
	TestCompositions.nodes
	VuoEditorApp
)
