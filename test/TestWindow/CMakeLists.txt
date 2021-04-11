VuoTest(NAME TestWindow
	SOURCE TestWindow.cc
	TIMEOUT 60

	# This test controls the mouse cursor
	# and expects its windows to be in the foreground without interruption.
	RUN_SERIAL ON
)
target_link_libraries(TestWindow
	PRIVATE
	"-framework CoreGraphics"
)
add_dependencies(TestWindow VuoEditorApp)
