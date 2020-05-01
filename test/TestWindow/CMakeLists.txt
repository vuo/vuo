VuoTest(NAME TestWindow
	SOURCE TestWindow.cc
	RESOURCE_LOCK GUI;COMPILER
	TIMEOUT 600
)
target_link_libraries(TestWindow
	PRIVATE
	"-framework CoreGraphics"
)
add_dependencies(TestWindow VuoEditorApp)
