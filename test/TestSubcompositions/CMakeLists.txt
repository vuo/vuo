VuoTest(NAME TestSubcompositions
	SOURCE TestSubcompositions.cc
	RESOURCE_LOCK COMPILER
)
target_link_libraries(TestSubcompositions
	PRIVATE
	VuoRenderer
	TestCompositionExecution
)
add_dependencies(TestSubcompositions VuoEditorApp)
