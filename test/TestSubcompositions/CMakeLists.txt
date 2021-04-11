VuoTest(NAME TestSubcompositions
	SOURCE TestSubcompositions.cc

	# This test installs modules to shared folders, which can trip up other tests.
	RUN_SERIAL ON
)
target_link_libraries(TestSubcompositions
	PRIVATE
	VuoRenderer
	TestCompositionExecution
)
add_dependencies(TestSubcompositions VuoEditorApp)
