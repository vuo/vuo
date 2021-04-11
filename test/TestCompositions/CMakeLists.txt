VuoTest(NAME TestCompositions
	SOURCE TestCompositions.cc
)
target_sources(TestCompositions
	PRIVATE
	PortConfiguration.cc
)
target_link_libraries(TestCompositions
	PRIVATE
		VuoRenderer
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
