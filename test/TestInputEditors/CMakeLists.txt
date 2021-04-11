VuoTest(NAME TestInputEditors
	SOURCE TestInputEditors.cc

	# This test sends keyboard events to the foreground window,
	# and expects its windows to be in the foreground without interruption.
	RUN_SERIAL ON
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
