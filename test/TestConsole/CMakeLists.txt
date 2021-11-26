VuoTest(NAME TestConsole
	SOURCE TestConsole.cc

	# This test uses the system-wide clipboard.
	RUN_SERIAL ON
)
set_target_properties(TestConsole PROPERTIES
	AUTORCC ON
)
target_sources(TestConsole
	PRIVATE
	../../editor/VuoEditorApp/VuoEditorApp.qrc
)
target_link_libraries(TestConsole
	PRIVATE
	VuoEditor
)
add_dependencies(TestConsole
	VuoEditorApp
)
