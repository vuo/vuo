VuoTest(NAME TestInputEditors
	SOURCE TestInputEditors.cc
	RESOURCE_LOCK GUI
)
target_link_libraries(TestInputEditors
	PRIVATE
	VuoEditor
	TestCompositionExecution
)
add_dependencies(TestInputEditors
	VuoInputEditors
	VuoEditorApp
)
