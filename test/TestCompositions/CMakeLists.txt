VuoTest(NAME TestCompositions
	SOURCE TestCompositions.cc
	FIXTURE TestCompositions
)
target_sources(TestCompositions
	PRIVATE
	PortConfiguration.cc
	PortConfiguration.hh
)
target_link_libraries(TestCompositions
	PRIVATE
		VuoRenderer
		TestCompositionExecution
)

VuoCompileNodes(
	node-TestCompositions/vuo.test.cxxException.cc
	node-TestCompositions/vuo.test.delay.c
	node-TestCompositions/vuo.test.mesh.c
	node-TestCompositions/vuo.test.wallTime.c
)
add_dependencies(TestCompositions
	TestCompositions.nodes
	VuoEditorApp
)

# Delete files created by these tests.
add_test(NAME TestCompositionsCleanup
	COMMAND bash -c "rm -f /tmp/TestCompositions*.mov || true"
)
set_tests_properties(TestCompositionsCleanup PROPERTIES
	FIXTURES_CLEANUP TestCompositions
)
