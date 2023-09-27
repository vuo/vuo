VuoTest(NAME TestSceneObjectGenerators
	SOURCE TestSceneObjectGenerators.cc
)
target_link_libraries(TestSceneObjectGenerators
	PRIVATE
	TestCompositionExecution
)
add_dependencies(TestSceneObjectGenerators
	VuoEditorApp
)
